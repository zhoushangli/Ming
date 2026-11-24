#include "MingEngine/Editor/EditorProjectChecker.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Core/IO/ResourceImporter.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <algorithm>
#include <system_error>

bool EditorProjectChecker::Stamp::operator==(Stamp const& other) const
{
	return m_time == other.m_time && m_size == other.m_size;
}

bool EditorProjectChecker::ReadStamp(std::filesystem::path const& path, Stamp& stamp)
{
	std::error_code error;
	stamp.m_time = std::filesystem::last_write_time(path, error);
	if (error)
	{
		return false;
	}

	stamp.m_size = std::filesystem::file_size(path, error);
	return !error;
}

EditorProjectChecker::EditorProjectChecker(EditorNode& editor) : m_editor(editor)
{
	m_root = std::filesystem::absolute(g_engine->m_fileSystem->GetResourceRoot()).lexically_normal();
	g_engine->m_fileSystem->SetEditorScanCallback([this]() { CheckProject(); });
	ResourceSaver::SetSavedCallback([this](VirtualPath const& path) { NotifyFileSaved(path); });
}

EditorProjectChecker::~EditorProjectChecker()
{
	Shutdown();
}

void EditorProjectChecker::Shutdown()
{
	if (m_stop.exchange(true))
	{
		return;
	}

	g_engine->m_fileSystem->SetEditorScanCallback({});
	ResourceSaver::SetSavedCallback({});
	if (m_task != WorkerThreadPool::INVALID_TASK_ID && g_engine->HasWorkerThreadPool())
	{
		g_engine->m_workerThreadPool->WaitForTaskCompletion(m_task);
	}
	m_task = WorkerThreadPool::INVALID_TASK_ID;
}

void EditorProjectChecker::CheckProject(bool full)
{
	if (!m_stop)
	{
		m_pending = true;
		m_full = m_full || full;
	}
}

bool EditorProjectChecker::IsBusy() const
{
	return !m_initialComplete || m_importing || m_choosing || !m_drops.empty();
}

bool EditorProjectChecker::IsInitialScanComplete() const
{
	return m_initialComplete;
}

void EditorProjectChecker::NotifyFileSaved(VirtualPath const& path)
{
	if (ResourceImporter::IsInternalResourcePath(path))
	{
		return;
	}

	std::filesystem::path physical;
	Stamp stamp;
	if (g_engine->m_fileSystem->TryGetPhysicalPath(path, physical) && ReadStamp(physical, stamp))
	{
		m_known[path.GetString()] = stamp;
		++m_revision;
		m_refreshTree = true;
	}
}

void EditorProjectChecker::NotifySceneLoaded(VirtualPath const& path)
{
	NotifyFileSaved(path);
}

void EditorProjectChecker::ScanTask(void* userdata)
{
	auto& self = *static_cast<EditorProjectChecker*>(userdata);
	try
	{
		std::error_code error;
		std::filesystem::recursive_directory_iterator it(self.m_root, std::filesystem::directory_options::skip_permission_denied, error), end;
		for (; it != end && !self.m_stop; it.increment(error))
		{
			if (error)
			{
				self.m_errors.push_back(error.message());
				error.clear();
				continue;
			}

			auto const path = it->path();
			auto const name = path.filename().u8string();
			if (it->is_symlink(error))
			{
				it.disable_recursion_pending();
				continue;
			}
			if (it->is_directory(error))
			{
				auto const relative = path.lexically_relative(self.m_root);
				bool const internal = !relative.empty() && *relative.begin() == ".ming";
				bool const importDirectory = relative == std::filesystem::path(".ming") || relative == std::filesystem::path(".ming/Import");
				if ((internal && !importDirectory) || name == ".git" || name == "bin" || name == "obj")
				{
					it.disable_recursion_pending();
				}
				else if (!internal)
				{
					self.m_directories.emplace_back("res://" + relative.generic_u8string());
				}
				continue;
			}

			auto const relative = path.lexically_relative(self.m_root);
			if (!relative.empty() && *relative.begin() == ".ming" && relative.parent_path() != std::filesystem::path(".ming/Import"))
			{
				continue;
			}
			Stamp stamp;
			if (ReadStamp(path, stamp))
			{
				self.m_scanned["res://" + path.lexically_relative(self.m_root).generic_u8string()] = stamp;
				++self.m_completed;
			}
		}
		if (error)
		{
			self.m_errors.push_back(error.message());
		}
	}
	catch (std::exception const& exception)
	{
		self.m_errors.push_back(exception.what());
	}
}

void EditorProjectChecker::StartScan()
{
	m_scanned.clear();
	m_directories.clear();
	m_errors.clear();
	m_completed = 0;
	m_pending = false;
	m_scanFull = m_full;
	m_full = false;
	m_scanRevision = m_revision;
	m_started = std::chrono::steady_clock::now();
	m_task = g_engine->m_workerThreadPool->AddNativeTask(ScanTask, this);
	if (m_task == WorkerThreadPool::INVALID_TASK_ID)
	{
		ERR_PRINT("Could not start project scan.");
		m_pending = true;
	}
}

void EditorProjectChecker::AcceptScan()
{
	if (m_scanRevision != m_revision || !m_errors.empty())
	{
		for (auto const& error : m_errors)
		{
			ERR_PRINT(error);
		}
		if (m_progressVisible)
		{
			m_editor.m_editorUI->HideBuildProgress();
			m_progressVisible = false;
		}
		if (!m_errors.empty())
		{
			m_editor.m_editorUI->Warning("Project Scan Failed", "Could not scan all project files. See Output. Use Refresh to retry.");
			m_initialComplete = true;
		}
		m_pending = m_scanRevision != m_revision;
		m_full = m_full || m_scanFull;
		return;
	}

	m_imports.clear();
	m_conflicts.clear();
	std::string const scene = m_editor.GetCurrentScenePath().GetString();
	for (auto const& entry : m_scanned)
	{
		VirtualPath path(entry.first);
		if (ResourceImporter::IsInternalResourcePath(path))
		{
			continue;
		}
		auto const previous = m_known.find(entry.first);
		bool const changed = previous == m_known.end() || !(previous->second == entry.second);
		if ((entry.first == scene || entry.first == ProjectSettings::GetSettingsPath()) && changed && m_initialComplete)
		{
			m_conflicts.push_back(entry.first);
			continue;
		}

		if (ResourceImporter::CanImport(path))
		{
			VirtualPath output;
			bool needsCheck = changed || m_scanFull || !m_initialComplete || !ResourceImporter::TryGetImportFile(path, output);
			for (auto const& dependency : { ResourceImporter::GetImportConfigPath(path).GetString(), output.GetString() })
			{
				auto const current = m_scanned.find(dependency);
				auto const old = m_known.find(dependency);
				needsCheck = needsCheck || current == m_scanned.end() || old == m_known.end()
					|| !(current->second == old->second);
			}
			if (needsCheck)
			{
				m_imports.push_back(entry.first);
			}
		}
		else if (changed && m_initialComplete && !ResourceImporter::IsImportConfigPath(path))
		{
			ResourceLoader::ReloadIfCached(path);
		}
	}

	for (auto const& entry : m_known)
	{
		if (m_scanned.count(entry.first) == 0 && !ResourceImporter::IsInternalResourcePath(VirtualPath(entry.first)))
		{
			ERR_PRINT("Project file is missing: " + entry.first);
		}
	}

	// 2) Preserve conflict baselines until the user has resolved the choice.
	auto previous = std::move(m_known);
	m_known = m_scanned;
	for (auto const& path : m_conflicts)
	{
		auto const found = previous.find(path);
		if (found != previous.end())
		{
			m_known[path] = found->second;
		}
	}

	if (!m_imports.empty())
	{
		m_editor.m_editorUI->FinishNativeEditing();
		m_editor.m_editorUI->GetPanel<ViewportPanel>().CancelNavigation();
		m_editor.m_editorGizmos->CancelDrag();
		auto* viewport = static_cast<Viewport*>(m_editor.GetSceneTree()->GetRoot());
		viewport->CancelGuiInteraction(viewport);
	}
	m_importIndex = 0;
	m_importing = true;
}

void EditorProjectChecker::FinishImports()
{
	m_importing = false;
	CommitTree();
	m_refreshTree = false;
	if (m_progressVisible)
	{
		m_editor.m_editorUI->HideBuildProgress();
		m_progressVisible = false;
	}

	bool const first = !m_initialComplete;
	m_initialComplete = true;
	if (first)
	{
		VirtualPath const path = ProjectSettings::Get()->m_startScenePath;
		if (path.IsValid() && !m_editor.LoadScene(path))
		{
			m_editor.m_editorUI->Warning("Open Scene Failed", path.GetString());
		}
	}
}

void EditorProjectChecker::ShowProgress(std::string const& message, int completed, int total)
{
	m_editor.m_editorUI->SetStatusMessage(message);
	if (std::chrono::steady_clock::now() - m_started >= std::chrono::milliseconds(250))
	{
		m_progressVisible = true;
		m_editor.m_editorUI->ShowProjectProgress(message, completed, total);
	}
}

void EditorProjectChecker::ResolveExternalChanges(bool reload)
{
	bool succeeded = true;
	for (auto const& name : m_conflicts)
	{
		VirtualPath const path(name);
		bool success = false;
		if (name == ProjectSettings::GetSettingsPath())
		{
			if (reload)
			{
				std::string error;
				Ref<ProjectSettings> fresh = ProjectSettings::Read(m_root, error);
				if (fresh.IsValid())
				{
					fresh->SetVirtualPath(path);
					success = ProjectSettings::Get()->CopyFrom(std::move(*fresh));
				}
			}
			else
			{
				success = ResourceSaver::Save(path, ProjectSettings::Get());
			}
		}
		else if (path == m_editor.GetCurrentScenePath())
		{
			success = reload ? m_editor.LoadScene(path) : m_editor.SaveSceneAs(path);
		}

		if (success)
		{
			NotifyFileSaved(path);
		}
		else
		{
			succeeded = false;
		}
	}

	m_conflicts.clear();
	m_choosing = false;
	if (!succeeded)
	{
		m_editor.m_editorUI->Warning("Reload Failed", "Some files could not be loaded or saved. Current editor data has been retained. See Output.");
	}
}

void EditorProjectChecker::Update()
{
	if (m_stop || m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay())
	{
		return;
	}

	auto* viewport = static_cast<Viewport*>(m_editor.GetSceneTree()->GetRoot());
	if (viewport->GetGuiPopup() != nullptr && !m_progressVisible)
	{
		return;
	}

	if (m_task != WorkerThreadPool::INVALID_TASK_ID)
	{
		if (!g_engine->m_workerThreadPool->IsTaskCompleted(m_task))
		{
			ShowProgress("Scanning files...", m_completed, 0);
			return;
		}

		g_engine->m_workerThreadPool->WaitForTaskCompletion(m_task);
		m_task = WorkerThreadPool::INVALID_TASK_ID;
		AcceptScan();
	}

	if (m_importing)
	{
		if (m_importIndex < m_imports.size())
		{
			VirtualPath const path(m_imports[m_importIndex++]);
			ShowProgress("Reimporting assets... " + path.GetString(), static_cast<int>(m_importIndex), static_cast<int>(m_imports.size()));
			Stamp before;
			std::filesystem::path physical;
			g_engine->m_fileSystem->TryGetPhysicalPath(path, physical);
			ReadStamp(physical, before);
			bool imported = false;
			if (ResourceImporter::EnsureImported(path, m_scanFull, &imported))
			{
				g_engine->m_fileSystem->RecordImport(path);
				if (imported)
				{
					ResourceLoader::ReloadIfCached(path);
				}
			}
			else
			{
				ERR_PRINT("Import failed: " + path.GetString());
			}
			Stamp current;
			if (ReadStamp(physical, current) && !(before == current))
			{
				CheckProject();
			}
			return;
		}

		FinishImports();
	}

	if (!m_conflicts.empty() && !m_editor.m_editorUI->IsProjectSettingsOpen())
	{
		std::string message = "The following files are newer on disk:\n";
		for (auto const& path : m_conflicts)
		{
			message += "\n" + path;
		}
		message += "\n\nWhat action should be taken?";
		m_choosing = true;
		m_editor.m_editorUI->ShowChoice("Files have been modified outside Ming", message,
			{ "Reload from disk", "Ignore external changes" }, [this](int choice) { ResolveExternalChanges(choice == 0); });
		return;
	}

	if (m_refreshTree)
	{
		CommitTree();
		m_refreshTree = false;
	}
	if (!m_drops.empty() && !m_choosing && !m_importing && m_task == WorkerThreadPool::INVALID_TASK_ID)
	{
		auto drop = std::move(m_drops.front());
		m_drops.erase(m_drops.begin());
		StartExternalCopy(drop.m_paths, drop.m_directory);
		return;
	}
	if (m_pending && !m_choosing)
	{
		StartScan();
	}
}

void EditorProjectChecker::PrepareCopy(std::filesystem::path const& source, std::filesystem::path const& target)
{
	std::error_code error;
	VirtualPath targetPath;
	if (!g_engine->m_fileSystem->TryToVirtualPath(target, targetPath))
	{
		m_errors.push_back("Copy target is outside the project: " + target.u8string());
		return;
	}
	if (std::filesystem::is_symlink(source, error))
	{
		return;
	}
	if (std::filesystem::is_directory(source, error))
	{
		std::filesystem::create_directories(target, error);
		for (std::filesystem::directory_iterator it(source, error), end; it != end && !error; it.increment(error))
		{
			PrepareCopy(it->path(), target / it->path().filename());
		}
	}
	else if (std::filesystem::is_regular_file(source, error))
	{
		m_pendingCopies.push_back({ source, target, false });
	}
	if (error)
	{
		m_errors.push_back(source.u8string() + ": " + error.message());
	}
}

void EditorProjectChecker::ImportExternal(std::vector<std::string> const& paths, Vector2 position)
{
	if (m_stop || m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay())
	{
		return;
	}

	auto& panel = m_editor.m_editorUI->GetPanel<FileSystemPanel>();
	Tree* tree = panel.GetTree();
	Vector2 const local = position - tree->GetGlobalPosition();
	if (!tree->GetLocalRect().IsPointInside(local))
	{
		return;
	}

	m_drops.push_back({ paths, panel.GetDropDirectory(local) });
}

void EditorProjectChecker::StartExternalCopy(std::vector<std::string> const& paths, VirtualPath const& target)
{
	std::filesystem::path directory;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(target, directory))
	{
		return;
	}

	m_copies.clear();
	m_pendingCopies.clear();
	m_errors.clear();
	for (auto const& name : paths)
	{
		std::error_code error;
		auto const source = std::filesystem::weakly_canonical(std::filesystem::u8path(name), error);
		auto const destination = std::filesystem::weakly_canonical(directory / source.filename(), error);
		auto const relative = destination.lexically_relative(source);
		if (error || source == destination || (!relative.empty() && *relative.begin() != ".."))
		{
			ERR_PRINT("Cannot copy a file or directory into itself: " + name);
			continue;
		}
		PrepareCopy(source, destination);
	}

	m_collisionIndex = 0;
	m_choosing = true;
	ContinueCopy();
}

void EditorProjectChecker::ContinueCopy()
{
	for (; m_collisionIndex < m_pendingCopies.size(); ++m_collisionIndex)
	{
		auto const file = m_pendingCopies[m_collisionIndex];
		std::error_code error;
		if (std::filesystem::exists(file.m_target, error))
		{
			m_editor.m_editorUI->ShowChoice("Replace File?", "A file already exists:\n" + file.m_target.u8string(),
				{ "Replace", "Skip", "Cancel" }, [this, file](int choice)
				{
					if (choice == 2)
					{
						m_choosing = false;
						m_copies.clear();
						CheckProject();
						return;
					}
					if (choice == 0)
					{
						auto replacement = file;
						replacement.m_replace = true;
						m_copies.push_back(replacement);
					}
					++m_collisionIndex;
					ContinueCopy();
				});
			return;
		}
		m_copies.push_back(file);
	}

	m_choosing = false;
	CopyFiles();
	CheckProject();
}

void EditorProjectChecker::CopyFiles()
{
	try
	{
		for (auto const& file : m_copies)
		{
			if (m_stop)
			{
				break;
			}
			m_editor.m_editorUI->SetStatusMessage("Copying files... " + file.m_target.filename().u8string());
			std::error_code error;
			std::filesystem::create_directories(file.m_target.parent_path(), error);
			if (!error)
			{
				std::filesystem::copy_file(file.m_source, file.m_target,
					file.m_replace ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none, error);
			}
			if (error)
			{
				m_errors.push_back(file.m_target.u8string() + ": " + error.message());
			}
		}
	}
	catch (std::exception const& exception)
	{
		m_errors.push_back(exception.what());
	}

	for (auto const& error : m_errors)
	{
		ERR_PRINT(error);
	}
	if (!m_errors.empty())
	{
		m_editor.m_editorUI->Warning("Copy Failed", "Some files could not be copied. See Output. Successfully copied files will be imported.");
	}
}

void EditorProjectChecker::CommitTree()
{
	std::unordered_map<VirtualPath, std::filesystem::file_time_type> files;
	for (auto const& entry : m_known)
	{
		files.emplace(VirtualPath(entry.first), entry.second.m_time);
	}
	g_engine->m_fileSystem->ApplyResourceSnapshot(m_directories, files);
}

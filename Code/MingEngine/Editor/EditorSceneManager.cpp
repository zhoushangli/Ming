#include "MingEngine/Editor/EditorSceneManager.hpp"
#include "MingEngine/Editor/EditorCSharpBuildManager.hpp"
#include "MingEngine/Editor/EditorRunManager.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

namespace
{
bool ContainsPath(VirtualPath const& directory, VirtualPath const& path)
{
	return path.IsValid() && (directory == path || path.GetString().rfind(directory.GetString() + "/", 0) == 0);
}
}

EditorSceneManager::EditorSceneManager(EditorNode& editor) : m_editor(editor)
{
}

void EditorSceneManager::RequestLoadScene(VirtualPath const& virtualPath)
{
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject())
	{
		return;
	}

	m_pendingScenePath = virtualPath;
	RequestSceneAction(PendingSceneAction::Load);
}

void EditorSceneManager::RequestCreateScene(
	VirtualPath const& virtualPath, std::string const& rootName, std::function<void()> completed)
{
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject())
	{
		return;
	}

	m_pendingScenePath     = virtualPath;
	m_pendingSceneRootName = rootName;
	RequestSceneAction(PendingSceneAction::Create, std::move(completed));
}

void EditorSceneManager::CancelPendingSceneAction()
{
	m_pendingSceneAction    = PendingSceneAction::None;
	m_waitingForSave        = false;
	m_resumePending         = false;
	m_openUnsavedScenePopup = false;
	m_afterClose            = {};
	m_editor.m_runManager->Cancel();
}

void EditorSceneManager::RequestSceneAction(PendingSceneAction action, std::function<void()> completed)
{
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject())
	{
		return;
	}

	m_editor.m_editorUI->FinishNativeEditing();
	CancelPendingSceneAction();
	m_pendingSceneAction = action;
	m_afterClose         = std::move(completed);
	if (IsSceneDirty())
	{
		m_openUnsavedScenePopup = true;
	}
	else
	{
		ExecutePendingSceneAction();
	}
}

void EditorSceneManager::RequestNewScene()
{
	RequestSceneAction(PendingSceneAction::New);
}

void EditorSceneManager::RequestCloseScene(std::function<void()> completed)
{
	RequestSceneAction(PendingSceneAction::Close, std::move(completed));
}

void EditorSceneManager::ClearSceneState()
{
	m_editor.m_editorGizmos->CancelDrag();
	m_editor.GetSceneTree()->ClearScene();
	m_editor.m_selection->Clear();
	m_editor.m_undoRedo.ClearAllHistory();
	m_editorData.m_currentScenePath = {};
	m_editorData.m_isSceneDirty     = false;
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
}

void EditorSceneManager::NewRootCreated()
{
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
	m_editorData.m_currentScenePath = {};
	m_editor.m_undoRedo.ClearAllHistory();
	MarkSceneDirty();
}

bool EditorSceneManager::SetMainScene(VirtualPath const& path)
{
	Ref<PackedScene> scene(ResourceLoader::LoadUncached(path));
	if (!scene.IsValid())
	{
		return false;
	}

	Ref<ProjectSettings> settings = ProjectSettings::Get();
	VirtualPath const    previous = settings->m_startScenePath;
	settings->m_startScenePath    = path;
	if (!ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings))
	{
		settings->m_startScenePath = previous;
		return false;
	}

	INFO_PRINT("Main scene: " + path.GetString());
	return true;
}

bool EditorSceneManager::LoadScene(VirtualPath const& virtualPath)
{
	Ref<Resource> loadedScene = ResourceLoader::LoadUncached(virtualPath);
	if (!loadedScene.IsValid())
	{
		return false;
	}

	Ref<PackedScene> packedScene(loadedScene);
	if (!packedScene.IsValid())
	{
		return false;
	}

	Node* newSceneRoot = packedScene->Instantiate();
	if (!newSceneRoot)
	{
		return false;
	}

	m_editor.m_editorGizmos->CancelDrag();
	m_editor.GetSceneTree()->ChangeScene(newSceneRoot);
	m_editor.m_undoRedo.ClearAllHistory();
	m_editorData.m_currentScenePath = virtualPath;
	m_editor.NotifySceneLoaded(virtualPath);
	m_editorData.m_isSceneDirty     = false;
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
	m_editor.m_selection->Clear();

	return true;
}

EditorSceneManager::SaveResult EditorSceneManager::SaveCurrentScene()
{
	m_editor.m_editorUI->FinishNativeEditing();
	if (!HasScene())
	{
		return SaveResult::Failed;
	}

	if (!m_editorData.m_currentScenePath.IsValid())
	{
		RequestSaveSceneAs();
		return SaveResult::AwaitingPath;
	}

	if (!SaveSceneAs(m_editorData.m_currentScenePath))
	{
		m_editor.m_editorUI->Warning("Save Failed", "Changes have not been discarded.");
		return SaveResult::Failed;
	}
	return SaveResult::Saved;
}

bool EditorSceneManager::SaveScene()
{
	m_lastSaveResult = SaveCurrentScene();
	return m_lastSaveResult == SaveResult::Saved;
}

bool EditorSceneManager::SaveSceneAs(VirtualPath const& path)
{
	m_editor.m_editorUI->FinishNativeEditing();
	Node* root = m_editor.GetSceneTree()->GetScene();
	if (root == nullptr || !path.IsValid() || !path.HasExtension(".tscn"))
	{
		return false;
	}

	Ref<PackedScene> scene = CreateRef<PackedScene>();
	if (!scene->Pack(root) || !ResourceSaver::Save(path, scene))
	{
		ERR_PRINT("Could not save scene: " + path.GetString());
		return false;
	}

	m_editorData.m_currentScenePath = path;
	m_editorData.m_isSceneDirty     = false;
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
	m_editorData.m_sceneWasSaved = true;
	g_engine->m_fileSystem->ScanResourceTree();
	INFO_PRINT("Saved scene: " + path.GetString());
	return true;
}

void EditorSceneManager::RequestSaveSceneAs()
{
	if (!HasScene())
	{
		return;
	}

	VirtualPath initial = m_editorData.m_currentScenePath;
	if (!initial.IsValid())
	{
		initial = VirtualPath("res://").Join("NewScene.tscn");
	}
	m_editor.m_editorUI->OpenSceneFile(
		SceneFilePopup::Mode::Save,
		initial,
		[this](VirtualPath const& path)
		{
			if (!SaveSceneAs(path))
			{
				CancelPendingSceneAction();
				return false;
			}

			m_resumePending  = m_waitingForSave;
			m_waitingForSave = false;
			return true;
		},
		[this]()
		{
			CancelPendingSceneAction();
		});
}

void EditorSceneManager::MarkSceneDirty()
{
	if (HasScene())
	{
		m_editorData.m_isSceneDirty = true;
	}
}

bool EditorSceneManager::IsSceneDirty() const
{
	return m_editorData.m_isSceneDirty;
}

bool EditorSceneManager::HasScene() const
{
	return m_editor.GetSceneTree() != nullptr && m_editor.GetSceneTree()->GetScene() != nullptr;
}

std::string EditorSceneManager::GetCurrentSceneName() const
{
	return m_editorData.m_currentScenePath.IsValid() ? m_editorData.m_currentScenePath.GetStem()
		   : HasScene()                              ? "Untitled"
													 : "No Scene";
}

bool EditorSceneManager::CreateScene(VirtualPath const& virtualPath, std::string const& rootName)
{
	Node3D* sceneRoot = MemNew<Node3D>();
	sceneRoot->SetName(rootName);
	Ref<PackedScene> packedScene = CreateRef<PackedScene>();
	if (!packedScene->Pack(sceneRoot) || !ResourceSaver::Save(virtualPath, packedScene))
	{
		MemDelete(sceneRoot);
		if (m_editor.m_editorUI != nullptr)
		{
			m_editor.m_editorUI->Warning("Create Scene Failed", virtualPath.GetString());
		}

		return false;
	}

	m_editor.m_editorGizmos->CancelDrag();
	m_editor.GetSceneTree()->ChangeScene(sceneRoot);
	m_editor.m_undoRedo.ClearAllHistory();
	m_editorData.m_currentScenePath = virtualPath;
	m_editor.NotifySceneLoaded(virtualPath);
	m_editorData.m_isSceneDirty     = false;
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
	m_editor.m_selection->Clear();
	if (g_engine->m_fileSystem != nullptr)
	{
		g_engine->m_fileSystem->ScanResourceTree();
	}

	return true;
}

void EditorSceneManager::RenderUnsavedScenePopup()
{
	if (m_openUnsavedScenePopup)
	{
		m_openUnsavedScenePopup = false;
		auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now() - m_editorData.m_sceneActionTime).count();
		std::string const path = m_editor.GetCurrentScenePath().IsValid() ? m_editor.GetCurrentScenePath().GetString() : GetCurrentSceneName();
		std::string const time = seconds < 60 ? std::to_string(seconds) + " seconds ago."
			: std::to_string(seconds / 60) + " minutes ago.";
		bool const quit = m_pendingSceneAction == PendingSceneAction::Quit;
		bool const close = m_pendingSceneAction == PendingSceneAction::Close;
		m_editor.m_editorUI->ShowUnsavedScene("Scene \"" + path + "\" has unsaved changes.\nScene "
			+ (m_editorData.m_sceneWasSaved ? "saved: " : "opened: ") + time,
			quit ? "Save before quitting?" : close ? "Save before closing?" : "Save before continuing?",
			quit ? "Save & Quit" : close ? "Save & Close" : "Save & Continue");
	}
}

void EditorSceneManager::ExecutePendingSceneAction()
{
	PendingSceneAction const action    = m_pendingSceneAction;
	auto                     completed = std::move(m_afterClose);
	m_pendingSceneAction               = PendingSceneAction::None;
	switch (action)
	{
	case PendingSceneAction::New:
	case PendingSceneAction::Close:
		ClearSceneState();
		if (action == PendingSceneAction::Close && completed)
		{
			completed();
		}
		break;
	case PendingSceneAction::Load:
		if (!LoadScene(m_pendingScenePath))
		{
			m_editor.m_editorUI->Warning("Open Scene Failed", m_pendingScenePath.GetString());
		}
		break;
	case PendingSceneAction::Create:
		if (CreateScene(m_pendingScenePath, m_pendingSceneRootName) && completed)
		{
			completed();
		}
		break;
	case PendingSceneAction::Quit:
		m_editorData.m_isSceneDirty = false;
		g_app->Quit();
		break;
	case PendingSceneAction::Continue:
		if (completed)
		{
			completed();
		}
		break;
	default:
		break;
	}
}

void EditorSceneManager::ResolvePendingSceneAction(SceneActionChoice choice)
{
	switch (choice)
	{
	case SceneActionChoice::Cancel:
		CancelPendingSceneAction();
		return;
	case SceneActionChoice::Save:
		if (!SaveScene())
		{
			m_waitingForSave = m_lastSaveResult == SaveResult::AwaitingPath;
			if (!m_waitingForSave)
			{
				CancelPendingSceneAction();
			}
			return;
		}
		break;
	case SceneActionChoice::Discard:
		break;
	}

	m_resumePending = true;
}

void EditorSceneManager::RequestQuit()
{
	if (m_editor.IsCheckingProject())
	{
		m_quitRequested = true;
		return;
	}

	m_quitRequested = false;
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject())
	{
		m_editor.m_runManager->Cancel();
		g_engine->m_scriptSystem->CloseProjectBuild();
		m_editor.m_buildManager->m_buildInProgress = false;
		m_editor.m_editorUI->HideBuildProgress();
		CancelPendingSceneAction();
	}

	if (m_pendingSceneAction != PendingSceneAction::Quit)
	{
		RequestSceneAction(PendingSceneAction::Quit);
	}
}

bool EditorSceneManager::ContainsCurrentScene(VirtualPath const& path) const
{
	return ContainsPath(path, m_editorData.m_currentScenePath);
}

void EditorSceneManager::FilesMoved(VirtualPath const& before, VirtualPath const& after)
{
	m_editor.m_editorUI->GetPanel<FileSystemPanel>().FilesMoved(before, after);
	auto update = [&](VirtualPath& path)
	{
		if (ContainsPath(before, path))
		{
			path = VirtualPath(after.GetString() + path.GetString().substr(before.GetString().size()));
			return true;
		}
		return false;
	};
	update(m_editorData.m_currentScenePath);
	Ref<ProjectSettings> settings = ProjectSettings::Get();
	if (update(settings->m_startScenePath) && !ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings))
	{
		ERR_PRINT("Files moved, but project.ming could not be saved. The updated main scene path remains in memory.");
		m_editor.m_editorUI->Warning(
			"Project Settings",
			"Files moved, but project.ming could not be saved. Save Project Settings to retry.");
	}
}

bool EditorSceneManager::FilesRemoved(VirtualPath const& path)
{
	Ref<ProjectSettings> settings = ProjectSettings::Get();
	if (!ContainsPath(path, settings->m_startScenePath))
	{
		return true;
	}

	settings->m_startScenePath = {};
	return ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings);
}

void EditorSceneManager::RequestOpenScene()
{
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject())
	{
		return;
	}

	m_editor.m_editorUI->OpenSceneFile(
		SceneFilePopup::Mode::Open,
		m_editor.GetCurrentScenePath(),
		[this](VirtualPath const& path)
		{
			RequestLoadScene(path);
			return true;
		});
}

void EditorSceneManager::Update()
{
	if (m_quitRequested && !m_editor.IsCheckingProject())
	{
		RequestQuit();
	}

	RenderUnsavedScenePopup();
	if (m_resumePending)
	{
		m_resumePending = false;
		ExecutePendingSceneAction();
	}
}

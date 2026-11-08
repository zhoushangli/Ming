#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <filesystem>
#include <cwchar>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetClassName

namespace
{
std::string FormatObjectID(ObjectID objectID)
{
	return Stringf("uid=%u index=%u", objectID.GetUID(), objectID.GetIndex());
}

std::filesystem::path FindVSCode()
{
	std::vector<std::filesystem::path> candidates;
	for (auto const* name : { L"Code.exe", L"code.cmd" })
	{
		DWORD const length = SearchPathW(nullptr, name, nullptr, 0, nullptr, nullptr);
		if (length == 0)
		{
			continue;
		}

		std::wstring buffer(length, L'\0');
		DWORD const written = SearchPathW(nullptr, name, nullptr, length, buffer.data(), nullptr);
		if (written == 0 || written >= length)
		{
			continue;
		}

		buffer.resize(written);
		std::filesystem::path const found(buffer);
		candidates.push_back(found.extension() == L".cmd" ? found.parent_path().parent_path() / L"Code.exe" : found);
	}

	for (auto const* variable : { L"LOCALAPPDATA", L"ProgramFiles", L"ProgramFiles(x86)" })
	{
		DWORD const length = GetEnvironmentVariableW(variable, nullptr, 0);
		if (length == 0)
		{
			continue;
		}

		std::wstring buffer(length, L'\0');
		DWORD const written = GetEnvironmentVariableW(variable, buffer.data(), length);
		if (written == 0 || written >= length)
		{
			continue;
		}

		buffer.resize(written);
		std::filesystem::path directory(buffer);
		if (std::wstring(variable) == L"LOCALAPPDATA")
		{
			directory /= L"Programs";
		}

		candidates.push_back(directory / L"Microsoft VS Code" / L"Code.exe");
	}

	for (auto const& candidate : candidates)
	{
		std::error_code error;
		if (std::filesystem::is_regular_file(candidate, error))
		{
			return candidate;
		}
	}

	return {};
}
} // namespace

EditorNode* EditorNode::s_instance = nullptr;

ObjectID EditorSelection::GetSelected() const
{
	return m_selectedNodeID;
}

void EditorSelection::SetSelected(ObjectID nodeID)
{
	if (m_selectedNodeID == nodeID)
	{
		return;
	}

	ObjectID const previousNodeID = m_selectedNodeID;
	m_selectedNodeID              = nodeID;

	if (!previousNodeID.IsValid() && m_selectedNodeID.IsValid())
	{
		DebugGizmos::AddMessage(
			Stringf("Selected Node: %s", FormatObjectID(m_selectedNodeID).c_str()),
			5.f,
			Color::White,
			Color::White);
		return;
	}

	if (previousNodeID.IsValid() && m_selectedNodeID.IsValid())
	{
		DebugGizmos::AddMessage(
			Stringf(
				"Selection Changed: %s -> %s",
				FormatObjectID(previousNodeID).c_str(),
				FormatObjectID(m_selectedNodeID).c_str()),
			5.f,
			Color::White,
			Color::White);
		return;
	}

	if (previousNodeID.IsValid() && !m_selectedNodeID.IsValid())
	{
		DebugGizmos::AddMessage("Selection Cleared", 5.f, Color::White, Color::White);
	}
}

void EditorSelection::Clear()
{
	SetSelected(ObjectID::Invalid);
}

EditorNode::EditorNode(float editorScale) : m_editorScale(editorScale)
{
	s_instance = this;
	SetSerializable(false);
	m_editorUI = new EditorUI();

	m_editorCamera = MemNew<EditorCamera>();
	m_editorCamera->SetName("EditorCamera");
	m_editorCamera->SetLocalPosition(Vector3(5.f, 5.f, 5.f));
	m_editorCamera->SetLocalOrientation(EulerAngles(-135.f, 45.f, 0.f));
	m_editorCamera->SetSerializable(false);
	AddNode(m_editorCamera);

	m_editorGizmos = MemNew<EditorGizmos>();
	m_editorGizmos->SetName("EditorGizmos");
	m_editorGizmos->SetSerializable(false);
	AddNode(m_editorGizmos);

	SetReady(true);
	SetProcess(true);
}

EditorNode::~EditorNode()
{
	if (g_engine != nullptr && g_engine->m_scriptSystem != nullptr)
	{
		g_engine->m_scriptSystem->CloseProjectBuild();
	}

	SetReportMessageCallback({});
	delete m_editorUI;
	m_editorUI = nullptr;

	if (s_instance == this)
	{
		s_instance = nullptr;
	}
}

EditorNode* EditorNode::Get()
{
	return s_instance;
}

bool EditorNode::HandleGuiInput(InputEvent const& event)
{
	return m_editorUI != nullptr && m_editorUI->HandleInput(event);
}

bool EditorNode::IsEditingNativeControl() const
{
	return IsBuildingCSharpProject() || IsPreparingPlay()
		|| (m_editorUI != nullptr && m_editorUI->IsNativeInputActive());
}

EditorSelection& EditorNode::GetSelection()
{
	return m_selection;
}

EditorSelection const& EditorNode::GetSelection() const
{
	return m_selection;
}

void EditorNode::OnMouseMove(Vector2 screenPos, [[maybe_unused]] Vector2 delta)
{
	if (m_editorGizmos == nullptr || m_editorCamera == nullptr)
	{
		return;
	}

	Camera3D* camera = m_editorCamera->GetCamera();
	if (camera == nullptr)
	{
		return;
	}

	if (m_editorGizmos->IsDragging())
	{
		m_editorGizmos->OnDrag(*camera, screenPos);
	}
	else
	{
		m_editorGizmos->OnMouseMove(*camera, screenPos);
	}
}

void EditorNode::OnMouseDown(int keyCode, Vector2 screenPos)
{
	if (keyCode != ToKeyCode(KeyCode::LeftMouse))
	{
		return;
	}

	if (m_editorCamera == nullptr)
	{
		return;
	}

	Camera3D* camera = m_editorCamera->GetCamera();
	if (camera == nullptr)
	{
		return;
	}

	// 1) Let hovered gizmos capture the click
	if (m_editorGizmos != nullptr && m_editorGizmos->BeginDragHovered(*camera, screenPos))
	{
		return;
	}

	// 2) Gizmo didn't eat — try scene selection
	Vector2 mousePos           = screenPos;
	Vector2 viewportDimensions = Vector2(g_engine->m_windowSystem->GetClientDimensions());
	if (m_editorUI != nullptr)
	{
		mousePos           = m_editorUI->ToViewportPos(screenPos);
		viewportDimensions = m_editorUI->GetViewportDimensions();
	}

	RaycastSpace3D*       raycastSpace = GetSceneTree()->GetRaycastSpace();
	RaycastQuery3D        raycastQuery = camera->BuildRaycastFromMouse(mousePos, viewportDimensions, 10000.f);
	RaycastResult3D const result       = raycastSpace->IntersectRay(raycastQuery);

	ObjectID const gizmoNode = m_editorGizmos->PickNode(*camera, screenPos,
		result.m_didImpact ? result.m_impactDist : 10000.f);
	if (gizmoNode.IsValid())
	{
		m_selection.SetSelected(gizmoNode);
		return;
	}

	if (result.m_didImpact)
	{
		m_selection.SetSelected(result.m_owner);
	}
	else
	{
		m_selection.Clear();
	}
}

void EditorNode::OnMouseUp(int keyCode, [[maybe_unused]] Vector2 screenPos)
{
	if (keyCode != ToKeyCode(KeyCode::LeftMouse))
	{
		return;
	}

	if (m_editorGizmos != nullptr && m_editorGizmos->IsDragging())
	{
		m_editorGizmos->OnEndDrag();
	}
}

void EditorNode::OnReady()
{
	m_uiContext.m_sceneTree  = GetSceneTree();
	m_uiContext.m_selection  = &m_selection;
	m_uiContext.m_editorUI   = m_editorUI;
	m_uiContext.m_fileSystem = g_engine->m_fileSystem;
	m_baseEditorTheme        = EditorThemeManager::CreateTheme();
	m_editorTheme            = EditorThemeManager::CreateScaledTheme(m_baseEditorTheme, m_editorScale);
	g_engine->m_windowSystem->SetTitleBarColors(
		m_editorTheme->GetColor("background", "Editor"),
		m_editorTheme->GetColor("font_color", "Control"));
	m_editorUI->InitializeNative(this);
	SubViewport* sceneViewport = m_editorUI->GetSceneView()->GetSubViewport();
	m_editorCamera->Reparent(sceneViewport);
	m_editorGizmos->Reparent(sceneViewport);
	m_editorCamera->GetCamera()->SetCurrent();
	SetReportMessageCallback(
		[this](char const* severity, std::string const& message)
		{
			std::lock_guard<std::mutex> lock(m_messageMutex);
			m_messages.emplace_back(severity, message);
		});
}

void EditorNode::OnProcess([[maybe_unused]] float deltaSeconds)
{
	UpdateCSharpBuild();

	std::vector<std::pair<std::string, std::string>> messages;
	{
		std::lock_guard<std::mutex> lock(m_messageMutex);
		messages.swap(m_messages);
	}
	for (auto const& message : messages)
	{
		OutputPanel::Level const level = message.first == "Info"      ? OutputPanel::Level::Info
										 : message.first == "Warning" ? OutputPanel::Level::Warning
																	  : OutputPanel::Level::Error;
		m_editorUI->GetPanel<OutputPanel>().AddMessage({ "", level, message.second });
		m_editorUI->SetStatusMessage(message.second);
	}
	m_editorUI->Render(m_uiContext);
	RenderUnsavedScenePopup();
	if (m_resumePending && m_playDelayFrames > 0)
	{
		--m_playDelayFrames;
	}
	else if (m_resumePending)
	{
		m_resumePending = false;
		ExecutePendingSceneAction();
	}

	m_editorUI->FinishInput(m_uiContext);
}

void EditorNode::RequestLoadScene(VirtualPath const& virtualPath)
{
	if (IsBuildingCSharpProject() || IsPreparingPlay())
	{
		return;
	}

	m_pendingScenePath = virtualPath;
	RequestSceneAction(PendingSceneAction::Load);
}

void EditorNode::RequestCreateScene(
	VirtualPath const& virtualPath, std::string const& rootName, std::function<void()> completed)
{
	if (IsBuildingCSharpProject() || IsPreparingPlay())
	{
		return;
	}

	m_pendingScenePath     = virtualPath;
	m_pendingSceneRootName = rootName;
	RequestSceneAction(PendingSceneAction::Create, std::move(completed));
}

void EditorNode::CancelPendingSceneAction()
{
	m_pendingSceneAction    = PendingSceneAction::None;
	m_waitingForSave        = false;
	m_resumePending         = false;
	m_openUnsavedScenePopup = false;
	m_afterClose            = {};
}

void EditorNode::RequestSceneAction(PendingSceneAction action, std::function<void()> completed)
{
	if (IsBuildingCSharpProject() || IsPreparingPlay())
	{
		return;
	}

	m_editorUI->FinishNativeEditing();
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

void EditorNode::RequestNewScene()
{
	RequestSceneAction(PendingSceneAction::New);
}

void EditorNode::RequestCloseScene(std::function<void()> completed)
{
	RequestSceneAction(PendingSceneAction::Close, std::move(completed));
}

void EditorNode::RequestPlay()
{
	if (g_app == nullptr || g_app->IsGameRunning() || IsPreparingPlay() || IsBuildingCSharpProject())
	{
		return;
	}

	m_editorGizmos->CancelDrag();
	m_editorUI->FinishNativeEditing();
	CancelPendingSceneAction();
	m_pendingSceneAction = PendingSceneAction::Play;
	if (HasScene() && !SaveScene())
	{
		m_waitingForSave = m_lastSaveResult == SaveResult::AwaitingPath;
		if (!m_waitingForSave)
		{
			CancelPendingSceneAction();
		}
		return;
	}

	ExecutePendingSceneAction();
}

void EditorNode::ClearSceneState()
{
	m_editorGizmos->CancelDrag();
	GetSceneTree()->ClearScene();
	m_selection.Clear();
	m_undoRedo.ClearAllHistory();
	m_editorData.m_currentScenePath = {};
	m_editorData.m_isSceneDirty     = false;
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
}

void EditorNode::NewRootCreated()
{
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
	m_editorData.m_currentScenePath = {};
	m_undoRedo.ClearAllHistory();
	MarkSceneDirty();
}

bool EditorNode::SetMainScene(VirtualPath const& path)
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

bool EditorNode::LoadScene(VirtualPath const& virtualPath)
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

	m_editorGizmos->CancelDrag();
	GetSceneTree()->ChangeScene(newSceneRoot);
	m_undoRedo.ClearAllHistory();
	m_editorData.m_currentScenePath = virtualPath;
	m_editorData.m_isSceneDirty     = false;
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
	m_selection.Clear();

	return true;
}

EditorNode::SaveResult EditorNode::SaveCurrentScene()
{
	m_editorUI->FinishNativeEditing();
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
		m_editorUI->Warning("Save Failed", "Changes have not been discarded.");
		return SaveResult::Failed;
	}
	return SaveResult::Saved;
}

bool EditorNode::SaveScene()
{
	m_lastSaveResult = SaveCurrentScene();
	return m_lastSaveResult == SaveResult::Saved;
}

bool EditorNode::SaveSceneAs(VirtualPath const& path)
{
	m_editorUI->FinishNativeEditing();
	Node* root = GetSceneTree()->GetScene();
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

void EditorNode::RequestSaveSceneAs()
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
	m_editorUI->OpenSceneFile(
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

void EditorNode::BindMethods()
{
	ClassDatabase::BindMethod("MarkSceneDirty", &EditorNode::MarkSceneDirty, {});
}

void EditorNode::MarkSceneDirty()
{
	if (HasScene())
	{
		m_editorData.m_isSceneDirty = true;
	}
}

bool EditorNode::IsSceneDirty() const
{
	return m_editorData.m_isSceneDirty;
}

bool EditorNode::HasScene() const
{
	return GetSceneTree() != nullptr && GetSceneTree()->GetScene() != nullptr;
}

std::string EditorNode::GetCurrentSceneName() const
{
	return m_editorData.m_currentScenePath.IsValid() ? m_editorData.m_currentScenePath.GetStem()
		   : HasScene()                              ? "Untitled"
													 : "No Scene";
}

bool EditorNode::CreateScene(VirtualPath const& virtualPath, std::string const& rootName)
{
	Node3D* sceneRoot = MemNew<Node3D>();
	sceneRoot->SetName(rootName);
	Ref<PackedScene> packedScene = CreateRef<PackedScene>();
	if (!packedScene->Pack(sceneRoot) || !ResourceSaver::Save(virtualPath, packedScene))
	{
		MemDelete(sceneRoot);
		if (m_editorUI != nullptr)
		{
			m_editorUI->Warning("Create Scene Failed", virtualPath.GetString());
		}

		return false;
	}

	m_editorGizmos->CancelDrag();
	GetSceneTree()->ChangeScene(sceneRoot);
	m_undoRedo.ClearAllHistory();
	m_editorData.m_currentScenePath = virtualPath;
	m_editorData.m_isSceneDirty     = false;
	m_editorData.m_sceneActionTime = std::chrono::steady_clock::now();
	m_editorData.m_sceneWasSaved = false;
	m_selection.Clear();
	if (g_engine->m_fileSystem != nullptr)
	{
		g_engine->m_fileSystem->ScanResourceTree();
	}

	return true;
}

void EditorNode::RenderUnsavedScenePopup()
{
	if (m_openUnsavedScenePopup)
	{
		m_openUnsavedScenePopup = false;
		auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now() - m_editorData.m_sceneActionTime).count();
		std::string const path = GetCurrentScenePath().IsValid() ? GetCurrentScenePath().GetString() : GetCurrentSceneName();
		std::string const time = seconds < 60 ? std::to_string(seconds) + " seconds ago."
			: std::to_string(seconds / 60) + " minutes ago.";
		bool const quit = m_pendingSceneAction == PendingSceneAction::Quit;
		bool const close = m_pendingSceneAction == PendingSceneAction::Close;
		m_editorUI->ShowUnsavedScene("Scene \"" + path + "\" has unsaved changes.\nScene "
			+ (m_editorData.m_sceneWasSaved ? "saved: " : "opened: ") + time,
			quit ? "Save before quitting?" : close ? "Save before closing?" : "Save before continuing?",
			quit ? "Save & Quit" : close ? "Save & Close" : "Save & Continue");
	}
}

void EditorNode::ExecutePendingSceneAction()
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
			m_editorUI->Warning("Open Scene Failed", m_pendingScenePath.GetString());
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
	case PendingSceneAction::Play:
	{
		VirtualPath const main = ProjectSettings::Get()->m_startScenePath;
		Ref<PackedScene>  scene;
		if (main.IsValid())
		{
			scene = Ref<PackedScene>(ResourceLoader::LoadUncached(main));
		}
		if (!scene.IsValid())
		{
			m_editorUI->OpenSceneFile(
				SceneFilePopup::Mode::MainScene,
				GetCurrentScenePath(),
				[this](VirtualPath const& path)
				{
					if (!SetMainScene(path))
					{
						return false;
					}
					m_pendingSceneAction = PendingSceneAction::Play;
					m_resumePending      = true;
					return true;
				});
			return;
		}

		m_editorUI->SetStatusMessage("Preparing project for Play...");
		m_pendingSceneAction = PendingSceneAction::BuildPlay;
		m_playDelayFrames = 1;
		m_resumePending = true;
		break;
	}
	case PendingSceneAction::BuildPlay:
	{
		bool const scripted = g_engine->m_scriptSystem->HasProjectScripts()
			|| g_engine->m_fileSystem->Exists("res://Game.csproj");
		if (scripted)
		{
			BeginCSharpBuild(true);
		}
		else
		{
			LaunchPreparedGame(false);
		}
		break;
	}
	case PendingSceneAction::LaunchPlay:
		LaunchPreparedGame(true);
		break;
	default:
		break;
	}
}

bool EditorNode::IsSceneNavigationActive() const
{
	return m_editorUI != nullptr && m_editorUI->IsSceneNavigationActive();
}

void EditorNode::ResolvePendingSceneAction(SceneActionChoice choice)
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

void EditorNode::RequestQuit()
{
	if (IsBuildingCSharpProject() || IsPreparingPlay())
	{
		m_runAfterBuild = false;
		g_engine->m_scriptSystem->CloseProjectBuild();
		m_buildInProgress = false;
		m_editorUI->HideBuildProgress();
		CancelPendingSceneAction();
	}

	if (m_pendingSceneAction != PendingSceneAction::Quit)
	{
		RequestSceneAction(PendingSceneAction::Quit);
	}
}

namespace
{
bool ContainsPath(VirtualPath const& directory, VirtualPath const& path)
{
	return path.IsValid() && (directory == path || path.GetString().rfind(directory.GetString() + "/", 0) == 0);
}
}

bool EditorNode::ContainsCurrentScene(VirtualPath const& path) const
{
	return ContainsPath(path, m_editorData.m_currentScenePath);
}

void EditorNode::FilesMoved(VirtualPath const& before, VirtualPath const& after)
{
	m_editorUI->GetPanel<FileSystemPanel>().FilesMoved(before, after);
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
		m_editorUI->Warning(
			"Project Settings",
			"Files moved, but project.ming could not be saved. Save Project Settings to retry.");
	}
}

bool EditorNode::FilesRemoved(VirtualPath const& path)
{
	Ref<ProjectSettings> settings = ProjectSettings::Get();
	if (!ContainsPath(path, settings->m_startScenePath))
	{
		return true;
	}

	settings->m_startScenePath = {};
	return ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings);
}

bool EditorNode::SetEditorScale(float scale)
{
	if (!EditorThemeManager::SaveScale(scale))
	{
		return false;
	}
	struct ScrollState
	{
		ObjectID m_id;
		Vector2  m_position;
	};
	std::vector<ScrollState>   scrolls;
	std::function<void(Node*)> collect = [&](Node* node)
	{
		if (node == m_editorUI->GetSceneView())
		{
			return;
		}
		if (auto* scroll = dynamic_cast<ScrollContainer*>(node))
		{
			scrolls.push_back({ scroll->GetObjectID(), Vector2(scroll->GetHorizontalScroll(), scroll->GetScroll()) });
		}
		for (Node* child : node->GetChildren())
		{
			collect(child);
		}
	};
	collect(m_editorUI->GetRootControl());
	for (Node* child : GetSceneTree()->GetRoot()->GetChildren())
	{
		if (dynamic_cast<PopupPanel*>(child) != nullptr)
		{
			collect(child);
		}
	}
	float const ratio = scale / m_editorScale;
	m_editorGizmos->CancelDrag();
	m_editorScale = scale;
	m_editorTheme = EditorThemeManager::CreateScaledTheme(m_baseEditorTheme, scale);
	ThemeDatabase::SetThemeContext(m_editorUI->GetRootControl(), { m_editorTheme });
	g_engine->m_windowSystem->SetTitleBarColors(
		m_editorTheme->GetColor("background", "Editor"),
		m_editorTheme->GetColor("font_color", "Control"));
	GetSceneTree()->FlushContainerUpdate();
	for (ScrollState const& state : scrolls)
	{
		if (auto* scroll = ObjectDatabase::GetInstance<ScrollContainer>(state.m_id))
		{
			scroll->SetHorizontalScroll(state.m_position.x * ratio);
			scroll->SetScroll(state.m_position.y * ratio);
		}
	}
	return true;
}

void EditorNode::RequestOpenScene()
{
	if (IsBuildingCSharpProject() || IsPreparingPlay())
	{
		return;
	}

	m_editorUI->OpenSceneFile(
		SceneFilePopup::Mode::Open,
		GetCurrentScenePath(),
		[this](VirtualPath const& path)
		{
			RequestLoadScene(path);
			return true;
		});
}

bool EditorNode::GenerateCSharpProject()
{
	if (IsBuildingCSharpProject() || IsPreparingPlay())
	{
		return false;
	}

	if (!g_engine->m_scriptSystem->EnsureProjectSolution())
	{
		m_editorUI->Warning("C# Project", "Could not generate the C# project. See Output.");
		return false;
	}
	INFO_PRINT("C# project generated.");
	return true;
}

bool EditorNode::OpenCSharpScript(VirtualPath const& path)
{
	auto fail = [this](std::string const& message)
	{
		ERR_PRINT(message);
		m_editorUI->Warning("C# Editor", message);
		return false;
	};

	std::filesystem::path script;
	std::filesystem::path project;
	std::error_code error;
	if (!path.HasExtension(".cs") || !g_engine->m_fileSystem->TryGetPhysicalPath(path, script)
		|| !std::filesystem::is_regular_file(script, error)
		|| !g_engine->m_fileSystem->TryGetPhysicalPath(VirtualPath("res://"), project))
	{
		return fail("Could not find the C# script in the current project.");
	}

	std::filesystem::path const executable = FindVSCode();
	if (executable.empty())
	{
		return fail("Could not find Visual Studio Code. Install it in the default location or add its bin folder to PATH.");
	}

	if (!g_engine->m_scriptSystem->EnsureProjectSolution())
	{
		return fail("Could not prepare the C# project. See Output.");
	}

	// Start VS Code as an editor even when Ming was launched from an Electron tool.
	// e.g. Remove ELECTRON_RUN_AS_NODE=1 from the child environment only.
	wchar_t* inherited = GetEnvironmentStringsW();
	if (inherited == nullptr)
	{
		return fail("Could not read the environment for Visual Studio Code.");
	}

	std::wstring environment;
	for (wchar_t const* entry = inherited; *entry != L'\0'; entry += std::wcslen(entry) + 1)
	{
		wchar_t const* key = L"ELECTRON_RUN_AS_NODE=";
		if (_wcsnicmp(entry, key, std::wcslen(key)) != 0)
		{
			environment.append(entry, std::wcslen(entry) + 1);
		}
	}

	environment.push_back(L'\0');
	FreeEnvironmentStringsW(inherited);

	std::wstring command = L"\"" + executable.wstring() + L"\" \"" + (project / L".").wstring()
		+ L"\" \"" + script.wstring() + L"\"";
	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
		environment.data(), project.c_str(), &startup, &process))
	{
		return fail("Could not start Visual Studio Code. Windows error: " + std::to_string(GetLastError()));
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}

bool EditorNode::BuildCSharpProject()
{
	if (IsBuildingCSharpProject() || IsPreparingPlay() || (g_app != nullptr && g_app->IsGameRunning()))
	{
		return false;
	}

	return BeginCSharpBuild(false);
}

bool EditorNode::BeginCSharpBuild(bool runAfterBuild)
{
	m_editorUI->FinishNativeEditing();
	m_editorUI->GetPanel<ViewportPanel>().CancelNavigation();
	m_editorGizmos->CancelDrag();
	auto* viewport = m_editorUI->GetRootControl()->GetViewport();
	viewport->CancelGuiInteraction(viewport);

	m_buildExitCode = -1;
	if (!g_engine->m_scriptSystem->EnsureProjectSolution() || !g_engine->m_scriptSystem->BeginProjectBuild())
	{
		g_engine->m_scriptSystem->PollProjectBuild(m_buildExitCode);
		g_engine->m_scriptSystem->CloseProjectBuild();
		CancelPendingSceneAction();
		m_editorUI->Warning("C# Build", "Build failed. See Output.");
		return false;
	}

	m_buildStarted = std::chrono::steady_clock::now();
	m_buildInProgress = true;
	m_runAfterBuild = runAfterBuild;
	if (runAfterBuild)
	{
		m_pendingSceneAction = PendingSceneAction::BuildPlay;
	}
	m_editorUI->SetStatusMessage("Checking and building C# project...");
	return true;
}

void EditorNode::UpdateCSharpBuild()
{
	if (!m_buildInProgress)
	{
		return;
	}

	auto* scripts = g_engine->m_scriptSystem;
	ScriptSystem::BuildState const state = scripts->PollProjectBuild(m_buildExitCode);
	if (state == ScriptSystem::BuildState::Running)
	{
		if (std::chrono::steady_clock::now() - m_buildStarted >= std::chrono::milliseconds(250))
		{
			m_editorUI->ShowBuildProgress("Checking and building C# project...");
		}
		return;
	}

	m_buildInProgress = false;
	bool const runAfterBuild = std::exchange(m_runAfterBuild, false);
	scripts->CloseProjectBuild();
	if (state != ScriptSystem::BuildState::Succeeded)
	{
		CancelPendingSceneAction();
		m_editorUI->HideBuildProgress();
		m_editorUI->SetStatusMessage("C# build failed.");
		m_editorUI->Warning("C# Build", "Build failed. See Output.");
		return;
	}

	INFO_PRINT("C# build completed.");
	if (runAfterBuild)
	{
		m_editorUI->SetStatusMessage("Loading C# assembly...");
		if (m_editorUI->IsBuildProgressVisible())
		{
			m_editorUI->ShowBuildProgress("Loading C# assembly...");
		}
		m_pendingSceneAction = PendingSceneAction::LaunchPlay;
		m_playDelayFrames = 1;
		m_resumePending = true;
	}
	else
	{
		m_editorUI->HideBuildProgress();
		m_editorUI->SetStatusMessage("C# build completed.");
	}
}

void EditorNode::LaunchPreparedGame(bool scripted)
{
	std::string error;
	if (scripted)
	{
		if (!g_engine->m_scriptSystem->ReloadProjectAssembly())
		{
			error = "Could not load the compiled C# assembly. See Output.";
		}
		else
		{
			Ref<PackedScene> main(ResourceLoader::LoadUncached(ProjectSettings::Get()->m_startScenePath));
			Node* root = main.IsValid() ? main->Instantiate() : nullptr;
			bool const valid = g_engine->m_scriptSystem->ValidateSceneScripts(root);
			MemDelete(root);
			if (!valid)
			{
				error = "A scene script has no compiled type or an incompatible base. See Output.";
			}
		}
	}

	m_editorUI->HideBuildProgress();
	if (error.empty() && !g_app->LaunchGame())
	{
		error = "Could not start the game. See Output for details.";
	}
	if (!error.empty())
	{
		m_editorUI->SetStatusMessage("Run failed.");
		m_editorUI->Warning("Run Failed", error);
	}
}

bool EditorNode::ReloadCSharpAssembly()
{
	if (IsBuildingCSharpProject() || IsPreparingPlay())
	{
		return false;
	}

	if (!g_engine->m_scriptSystem->ReloadProjectAssembly())
	{
		m_editorUI->Warning("C# Reload", "Reload failed. See Output.");
		return false;
	}
	INFO_PRINT("C# assembly reloaded.");
	return true;
}

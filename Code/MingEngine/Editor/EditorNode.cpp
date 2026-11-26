#include "MingEngine/Editor/EditorChecker.hpp"
#include "MingEngine/Editor/EditorRunManager.hpp"
#include "MingEngine/Editor/EditorCSharpBuildManager.hpp"
#include "MingEngine/Editor/EditorSceneManager.hpp"
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

EditorNode* EditorNode::s_instance = nullptr;

EditorNode::EditorNode(float editorScale, std::function<void()> pollGameProcess)
	: m_editorScale(editorScale), m_pollGameProcess(std::move(pollGameProcess))
{
	s_instance = this;
	m_selection = new EditorSelection();
	SetSerializable(false);
	m_editorUI = new EditorUI();
	m_runManager = new EditorRunManager(*this);
	m_buildManager = new EditorCSharpBuildManager(*this);
	m_sceneManager = new EditorSceneManager(*this);

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
	ShutdownBackgroundWork();
	delete m_checker;
	delete m_runManager;
	delete m_buildManager;
	delete m_sceneManager;
	SetReportMessageCallback({});
	delete m_editorUI;
	m_editorUI = nullptr;
	delete m_selection;
	m_selection = nullptr;

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
	if (event.m_type == InputEventType::WindowFocus && event.m_focused)
	{
		CheckProject();
	}
	return m_editorUI != nullptr && m_editorUI->HandleInput(event);
}

bool EditorNode::IsEditingNativeControl() const
{
	return IsBuildingCSharpProject() || IsPreparingPlay()
		|| (m_editorUI != nullptr && m_editorUI->IsNativeInputActive());
}

EditorSelection& EditorNode::GetSelection()
{
	return *m_selection;
}

EditorSelection const& EditorNode::GetSelection() const
{
	return *m_selection;
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
		m_selection->SetSelected(gizmoNode);
		return;
	}

	if (result.m_didImpact)
	{
		m_selection->SetSelected(result.m_owner);
	}
	else
	{
		m_selection->Clear();
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

void EditorNode::_Ready()
{
	m_uiContext.m_sceneTree  = GetSceneTree();
	m_uiContext.m_selection  = m_selection;
	m_uiContext.m_editorUI   = m_editorUI;
	m_uiContext.m_fileSystem = g_engine->m_fileSystem;
	m_baseEditorTheme        = EditorThemeManager::CreateTheme();
	m_editorTheme            = EditorThemeManager::CreateScaledTheme(m_baseEditorTheme, m_editorScale);
	g_engine->m_windowSystem->SetTitleBarColors(
		m_editorTheme->GetColor("background", "Editor"),
		m_editorTheme->GetColor("font_color", "Control"));
	m_editorUI->InitializeNative(this);
	m_checker = new EditorChecker(*this);
	g_engine->m_windowSystem->SetFilesDroppedCallback([this](std::vector<std::string> const& paths, Vector2 position)
	{
		ImportExternalFiles(paths, position);
	});
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

void EditorNode::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Process:
	{
		if (m_pollGameProcess)
		{
			m_pollGameProcess();
		}

		if (!g_engine->m_inputSystem->IsWindowFocused())
		{
			g_engine->m_inputSystem->SetCursorMode(CursorMode::POINTER);
		}

		m_checker->Update();
		m_buildManager->UpdateCSharpBuild();

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
		m_sceneManager->Update();
		m_runManager->Update();

		m_editorUI->FinishInput(m_uiContext);
		break;
	}
	}
}

void EditorNode::RequestLoadScene(VirtualPath const& virtualPath)
{
	m_sceneManager->RequestLoadScene(virtualPath);
}

void EditorNode::RequestCreateScene(
	VirtualPath const& virtualPath, std::string const& rootName, std::function<void()> completed)
{
	m_sceneManager->RequestCreateScene(virtualPath, rootName, completed);
}

void EditorNode::RequestNewScene()
{
	m_sceneManager->RequestNewScene();
}

void EditorNode::RequestCloseScene(std::function<void()> completed)
{
	m_sceneManager->RequestCloseScene(completed);
}

void EditorNode::RequestPlay()
{
	m_runManager->RequestPlay();
}

void EditorNode::NewRootCreated()
{
	m_sceneManager->NewRootCreated();
}

bool EditorNode::SetMainScene(VirtualPath const& path)
{
	return m_sceneManager->SetMainScene(path);
}

bool EditorNode::LoadScene(VirtualPath const& virtualPath)
{
	return m_sceneManager->LoadScene(virtualPath);
}

bool EditorNode::SaveScene()
{
	return m_sceneManager->SaveScene();
}

bool EditorNode::SaveSceneAs(VirtualPath const& path)
{
	return m_sceneManager->SaveSceneAs(path);
}

void EditorNode::RequestSaveSceneAs()
{
	m_sceneManager->RequestSaveSceneAs();
}

void EditorNode::BindMethods()
{
	ClassDatabase::BindMethod("MarkResourceDirty", &EditorNode::MarkResourceDirty, { "resource" });
	ClassDatabase::BindMethod("MarkSceneDirty", &EditorNode::MarkSceneDirty, {});
}

void EditorNode::MarkSceneDirty()
{
	m_sceneManager->MarkSceneDirty();
}

bool EditorNode::IsSceneDirty() const
{
	return m_sceneManager->IsSceneDirty();
}

bool EditorNode::HasScene() const
{
	return m_sceneManager->HasScene();
}

std::string EditorNode::GetCurrentSceneName() const
{
	return m_sceneManager->GetCurrentSceneName();
}

bool EditorNode::IsSceneNavigationActive() const
{
	return m_editorUI != nullptr && m_editorUI->IsSceneNavigationActive();
}

void EditorNode::ResolvePendingSceneAction(SceneActionChoice choice)
{
	m_sceneManager->ResolvePendingSceneAction(choice);
}

void EditorNode::RequestQuit()
{
	m_sceneManager->RequestQuit();
}

bool EditorNode::ContainsCurrentScene(VirtualPath const& path) const
{
	return m_sceneManager->ContainsCurrentScene(path);
}

void EditorNode::FilesMoved(VirtualPath const& before, VirtualPath const& after)
{
	m_sceneManager->FilesMoved(before, after);
}

bool EditorNode::FilesRemoved(VirtualPath const& path)
{
	return m_sceneManager->FilesRemoved(path);
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
	m_sceneManager->RequestOpenScene();
}

bool EditorNode::GenerateCSharpProject()
{
	return m_buildManager->GenerateCSharpProject();
}

bool EditorNode::OpenCSharpScript(VirtualPath const& path)
{
	return m_buildManager->OpenCSharpScript(path);
}

bool EditorNode::BuildCSharpProject()
{
	return m_buildManager->BuildCSharpProject();
}

bool EditorNode::ReloadCSharpAssembly()
{
	return m_buildManager->ReloadCSharpAssembly();
}

bool EditorNode::IsPreparingPlay() const
{
	return m_runManager->IsPreparingPlay();
}

bool EditorNode::IsBuildingCSharpProject() const
{
	return m_buildManager->m_buildInProgress;
}

int32_t EditorNode::GetCSharpBuildExitCode() const
{
	return m_buildManager->m_buildExitCode;
}

VirtualPath EditorNode::GetCurrentScenePath() const
{
	return m_sceneManager->m_editorData.m_currentScenePath;
}

void EditorNode::CheckProject(bool full)
{
	if (m_checker != nullptr)
	{
		m_checker->CheckProject(full);
	}
}

void EditorNode::NotifyFileSaved(VirtualPath const& path)
{
	if (m_checker != nullptr)
	{
		m_checker->NotifyFileSaved(path);
	}
}

void EditorNode::NotifySceneLoaded(VirtualPath const& path)
{
	if (m_checker != nullptr)
	{
		m_checker->NotifySceneLoaded(path);
	}
}

bool EditorNode::IsCheckingProject() const
{
	return m_checker != nullptr && m_checker->IsBusy();
}

void EditorNode::ShutdownBackgroundWork()
{
	if (m_checker != nullptr)
	{
		m_checker->Shutdown();
	}
	g_engine->m_windowSystem->SetFilesDroppedCallback({});
}

void EditorNode::ImportExternalFiles(std::vector<std::string> const& paths, Vector2 position)
{
	if (m_checker != nullptr)
	{
		m_checker->ImportExternal(paths, position);
	}
}

void EditorNode::RequestStop()
{
	m_runManager->Stop();
}

void EditorNode::MarkResourceDirty(Object* object)
{
	if (m_checker != nullptr)
	{
		m_checker->MarkResourceDirty(dynamic_cast<Resource*>(object));
	}
}

bool EditorNode::SaveAll()
{
	m_editorUI->FinishNativeEditing();
	if (m_checker != nullptr && !m_checker->SavePendingChanges())
	{
		return false;
	}

	return !IsSceneDirty() || SaveScene();
}

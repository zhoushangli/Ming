#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace
{
std::string FormatNodeHandle(NodeHandle handle)
{
	return Stringf("uid=%u index=%u", handle.GetUID(), handle.GetIndex());
}

#ifdef _WIN32
struct FindMainWindowData
{
	DWORD pid;
	HWND  hwnd;
};

static BOOL CALLBACK FindMainWindowProc(HWND hwnd, LPARAM lParam)
{
	auto* data = reinterpret_cast<FindMainWindowData*>(lParam);
	DWORD pid  = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == data->pid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == NULL)
	{
		data->hwnd = hwnd;
		return FALSE;
	}
	return TRUE;
}
#endif

} // namespace

EditorNode* EditorNode::s_instance = nullptr;

NodeHandle EditorSelection::GetSelected() const { return m_selectedNodeHandle; }

void EditorSelection::SetSelected(NodeHandle handle)
{
	if (m_selectedNodeHandle == handle)
	{
		return;
	}

	NodeHandle const previousHandle = m_selectedNodeHandle;
	m_selectedNodeHandle            = handle;

	if (!previousHandle.IsValid() && m_selectedNodeHandle.IsValid())
	{
		DebugGizmos::AddMessage(
			Stringf("Selected Node: %s", FormatNodeHandle(m_selectedNodeHandle).c_str()),
			5.f,
			Color::White,
			Color::White);
		return;
	}

	if (previousHandle.IsValid() && m_selectedNodeHandle.IsValid())
	{
		DebugGizmos::AddMessage(
			Stringf(
				"Selection Changed: %s -> %s",
				FormatNodeHandle(previousHandle).c_str(),
				FormatNodeHandle(m_selectedNodeHandle).c_str()),
			5.f,
			Color::White,
			Color::White);
		return;
	}

	if (previousHandle.IsValid() && !m_selectedNodeHandle.IsValid())
	{
		DebugGizmos::AddMessage("Selection Cleared", 5.f, Color::White, Color::White);
	}
}

void EditorSelection::Clear() { SetSelected(NodeHandle::Invalid); }

EditorNode::EditorNode()
{
	s_instance = this;
	m_editorUI = new EditorUI();

	m_editorCamera = new EditorCamera();
	m_editorCamera->SetName("EditorCamera");
	m_editorCamera->SetLocalPosition(Vec3(5.f, 5.f, 5.f));
	m_editorCamera->SetLocalOrientation(EulerAngles(-135.f, 45.f, 0.f));
	m_editorCamera->SetSerializable(false);
	AddNode(m_editorCamera);

	m_editorGizmos = new EditorGizmos();
	m_editorGizmos->SetName("EditorGizmos");
	m_editorGizmos->SetSerializable(false);
	AddNode(m_editorGizmos);

	SetReady(true);
	SetProcess(true);
}

EditorNode::~EditorNode()
{
	// 1) Ensure the game process is stopped before editor shutdown.
	StopScene();

	delete m_editorUI;
	m_editorUI = nullptr;

	if (s_instance == this)
	{
		s_instance = nullptr;
	}
}

EditorNode* EditorNode::Get() { return s_instance; }

EditorSelection& EditorNode::GetSelection() { return m_selection; }

EditorSelection const& EditorNode::GetSelection() const { return m_selection; }

void EditorNode::OnMouseMove(Vec2 screenPos, [[maybe_unused]] Vec2 delta)
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

void EditorNode::OnMouseDown(int keyCode, Vec2 screenPos)
{
	if (!(g_engine->m_imguiSystem->WantCaptureMouse() && m_uiContext.m_isViewportImageHovered))
	{
		return;
	}

	if (keyCode != ToKeyCode(KeyCode::LeftMouse))
	{
		return;
	}

	if (m_editorGizmos == nullptr || m_editorCamera == nullptr)
	{
		return;
	}

	Camera3D* camera = m_editorCamera->GetCamera();
	if (camera == nullptr)
	{
		return;
	}

	// 1) Let hovered gizmos capture the click
	if (m_editorGizmos->IsHovered())
	{
		m_editorGizmos->BeginDragHovered(*camera, screenPos);
		return;
	}

	// 2) Gizmo didn't eat — try scene selection
	Vec2 mousePos           = screenPos;
	Vec2 viewportDimensions = Vec2(g_engine->m_windowSystem->GetClientDimensions());
	if (m_editorUI != nullptr)
	{
		mousePos           = m_editorUI->ToViewportPos(screenPos);
		viewportDimensions = m_editorUI->GetViewportDimensions();
	}

	RaycastSpace3D*       raycastSpace = GetSceneTree()->GetRaycastSpace();
	RaycastQuery3D        raycastQuery = camera->BuildRaycastFromMouse(mousePos, viewportDimensions, 10000.f);
	RaycastResult3D const result       = raycastSpace->IntersectRay(raycastQuery);

	if (result.m_didImpact)
	{
		m_selection.SetSelected(result.m_owner);
	}
	else
	{
		m_selection.Clear();
	}
}

void EditorNode::OnMouseUp(int keyCode, [[maybe_unused]] Vec2 screenPos)
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
}

void EditorNode::OnProcess([[maybe_unused]] float deltaSeconds)
{
	if (m_editorUI != nullptr)
	{
		m_uiContext.m_isViewportImageHovered = false;

		m_editorUI->Render(m_uiContext);
		RenderUnsavedScenePopup();
	}

	InputSystem* input       = g_engine->m_inputSystem;
	bool const   controlDown = input->IsKeyDown(KeyCode::LeftControl) || input->IsKeyDown(KeyCode::RightControl);
	if (controlDown && input->WasKeyJustPressed(KeyCode::S) && HasScene())
	{
		if (!SaveScene() && m_editorUI != nullptr)
		{
			m_editorUI->Warning("Save Scene Failed", m_editorData.m_currentScenePath.GetString());
		}
	}

	// 1) Check if the game process exited on its own
	CheckPIEProcessAlive();

	// 2) Dispatch mouse events when in Pointer mode
	if (m_editorCamera != nullptr && m_editorCamera->GetControlState() == EditorCamera::EditorControlState::Pointer)
	{
		Vec2 const cursorPos = m_editorCamera->GetCursorClientPos();
		Vec2 const delta     = m_editorCamera->GetCursorDelta();

		OnMouseMove(cursorPos, delta);

		if (input->WasKeyJustPressed(KeyCode::LeftMouse))
		{
			OnMouseDown(ToKeyCode(KeyCode::LeftMouse), cursorPos);
		}

		if (input->WasKeyJustReleased(KeyCode::LeftMouse))
		{
			OnMouseUp(ToKeyCode(KeyCode::LeftMouse), cursorPos);
		}
	}
}

void EditorNode::RequestLoadScene(VirtualPath const& virtualPath)
{
	if (!m_editorData.m_isSceneDirty)
	{
		if (!LoadScene(virtualPath) && m_editorUI != nullptr)
		{
			m_editorUI->Warning("Open Scene Failed", virtualPath.GetString());
		}
		return;
	}

	// Store the requested load so the confirmation popup can resume it.
	// e.g. ExecutePendingSceneAction() loads m_pendingScenePath after confirmation.
	m_pendingSceneAction = PendingSceneAction::Load;
	m_pendingScenePath   = virtualPath;
	m_pendingSceneRootName.clear();
	m_openUnsavedScenePopup = true;
}

void EditorNode::RequestCreateScene(VirtualPath const& virtualPath, std::string const& rootName)
{
	if (!m_editorData.m_isSceneDirty)
	{
		CreateScene(virtualPath, rootName);
		return;
	}

	// Store the requested creation so no file is written before confirmation.
	// e.g. ExecutePendingSceneAction() creates m_pendingScenePath after confirmation.
	m_pendingSceneAction    = PendingSceneAction::Create;
	m_pendingScenePath      = virtualPath;
	m_pendingSceneRootName  = rootName;
	m_openUnsavedScenePopup = true;
}

bool EditorNode::LoadScene(VirtualPath const& virtualPath)
{
	Ref<Resource> loadedScene = ResourceLoader::Load(virtualPath);
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

	GetSceneTree()->ChangeScene(newSceneRoot);
	m_editorData.m_currentScenePath = virtualPath;
	m_editorData.m_isSceneDirty     = false;
	m_selection.Clear();

	return true;
}

bool EditorNode::SaveScene()
{
	SceneTree* sceneTree = GetSceneTree();
	Node*      sceneRoot = sceneTree->GetScene();
	if (sceneRoot == nullptr || !m_editorData.m_currentScenePath.IsValid())
	{
		return false;
	}
	Ref<PackedScene> packedScene = CreateRef<PackedScene>();

	if (!packedScene->Pack(sceneRoot))
	{
		return false;
	}

	bool const result = ResourceSaver::Save(m_editorData.m_currentScenePath, packedScene);
	if (result)
	{
		m_editorData.m_isSceneDirty = false;
	}

	return result;
}

void EditorNode::MarkSceneDirty()
{
	if (HasScene())
	{
		m_editorData.m_isSceneDirty = true;
	}
}

bool EditorNode::IsSceneDirty() const { return m_editorData.m_isSceneDirty; }

bool EditorNode::HasScene() const { return GetSceneTree() != nullptr && GetSceneTree()->GetScene() != nullptr; }

std::string EditorNode::GetCurrentSceneName() const
{
	return !m_editorData.m_currentScenePath.IsValid()
			   ? std::string()
			   : m_editorData.m_currentScenePath.GetStem();
}

bool EditorNode::CreateScene(VirtualPath const& virtualPath, std::string const& rootName)
{
	Node3D* sceneRoot = new Node3D();
	sceneRoot->SetName(rootName);
	Ref<PackedScene> packedScene = CreateRef<PackedScene>();
	if (!packedScene->Pack(sceneRoot) || !ResourceSaver::Save(virtualPath, packedScene))
	{
		delete sceneRoot;
		if (m_editorUI != nullptr)
		{
			m_editorUI->Warning("Create Scene Failed", virtualPath.GetString());
		}
		return false;
	}

	GetSceneTree()->ChangeScene(sceneRoot);
	m_editorData.m_currentScenePath = virtualPath;
	m_editorData.m_isSceneDirty     = false;
	m_selection.Clear();
	if (g_engine->m_fileSystem != nullptr)
	{
		g_engine->m_fileSystem->ScanResourceTree();
	}
	return true;
}

void EditorNode::RenderUnsavedScenePopup()
{
	constexpr char const* popupId = "Please Confirm...";
	if (m_openUnsavedScenePopup)
	{
		ImGui::OpenPopup(popupId);
		m_openUnsavedScenePopup = false;
	}
	if (!EditorPopupUtils::BeginModal(popupId, ImVec2(560.f, 0.f), ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	ImGui::TextUnformatted(
		!m_editorData.m_currentScenePath.IsValid() ? "This scene was never saved." : "This scene has unsaved changes.");
	ImGui::Dummy(ImVec2(0.f, 12.f));
	ImGui::TextUnformatted("Save before closing?");
	ImGui::Dummy(ImVec2(0.f, 12.f));

	bool const saveAndClose = ImGui::Button("Save & Close", ImVec2(130.f, 0.f));
	ImGui::SameLine(0.f, 40.f);
	bool const cancel = ImGui::Button("Cancel", ImVec2(130.f, 0.f));
	ImGui::SameLine(0.f, 40.f);
	bool const dontSave = ImGui::Button("Don't Save", ImVec2(130.f, 0.f));

	if (saveAndClose)
	{
		if (SaveScene())
		{
			ImGui::CloseCurrentPopup();
			ExecutePendingSceneAction();
		}
		else if (m_editorUI != nullptr)
		{
			m_editorUI->Warning("Save Scene Failed", m_editorData.m_currentScenePath.GetString());
		}
	}
	else if (cancel)
	{
		m_pendingSceneAction = PendingSceneAction::None;
		m_pendingScenePath = {};
		m_pendingSceneRootName.clear();
		ImGui::CloseCurrentPopup();
	}
	else if (dontSave)
	{
		ImGui::CloseCurrentPopup();
		ExecutePendingSceneAction();
	}

	EditorPopupUtils::EndModal();
}

void EditorNode::ExecutePendingSceneAction()
{
	// Execute and clear the deferred scene operation selected before the popup.
	// e.g. a pending load resumes after Save & Close or Don't Save.
	PendingSceneAction const action   = m_pendingSceneAction;
	VirtualPath const        path     = std::move(m_pendingScenePath);
	std::string const        rootName = std::move(m_pendingSceneRootName);
	m_pendingSceneAction              = PendingSceneAction::None;

	bool const result = action == PendingSceneAction::Load
							? LoadScene(path)
							: action == PendingSceneAction::Create && CreateScene(path, rootName);
	if (!result && action != PendingSceneAction::None && m_editorUI != nullptr)
	{
		m_editorUI->Warning(
			action == PendingSceneAction::Load ? "Open Scene Failed" : "Create Scene Failed", path.GetString());
	}
}

bool EditorNode::IsPlaying() const { return m_pieProcessHandle != nullptr; }

void EditorNode::PlayScene()
{
	// 1) Already playing
	if (IsPlaying())
	{
		return;
	}

	// 2) Save the current scene so the game loads the latest content
	SaveScene();

#ifdef _WIN32
	// 3) Get the current editor executable path
	wchar_t exePath[MAX_PATH];
	DWORD   pathLen = GetModuleFileNameW(NULL, exePath, MAX_PATH);
	if (pathLen == 0 || pathLen >= MAX_PATH)
	{
		if (m_editorUI != nullptr)
		{
			m_editorUI->Warning("PIE Failed", "Cannot get executable path.");
		}
		return;
	}

	std::wstring gamePath(exePath);

	// 4) Replace "Editor" with "Game" in the full path
	// e.g. Chess3D_Editor_Debug_x64.exe -> Chess3D_Game_Debug_x64.exe
	size_t pos = 0;
	while ((pos = gamePath.find(L"Editor", pos)) != std::wstring::npos)
	{
		gamePath.replace(pos, 6, L"Game");
		pos += 4;
	}

	// 5) Launch the game process (no extra args — it runs the start scene)
	STARTUPINFOW        si = { sizeof(si) };
	PROCESS_INFORMATION pi = {};

	if (CreateProcessW(gamePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		m_pieProcessHandle = pi.hProcess;
		m_pieProcessId     = pi.dwProcessId;
		CloseHandle(pi.hThread);
	}
	else
	{
		if (m_editorUI != nullptr)
		{
			m_editorUI->Warning("PIE Failed", "Cannot launch game executable. Make sure the Game build exists.");
		}
	}
#else
	if (m_editorUI != nullptr)
	{
		m_editorUI->Warning("PIE Failed", "PIE is only supported on Windows.");
	}
#endif
}

void EditorNode::StopScene()
{
	// 1) Guard: not playing
	if (!IsPlaying())
	{
		return;
	}

#ifdef _WIN32
	HANDLE processHandle = static_cast<HANDLE>(m_pieProcessHandle);

	// 2) Find the game window HWND by process ID
	FindMainWindowData data = { m_pieProcessId, NULL };
	EnumWindows(FindMainWindowProc, reinterpret_cast<LPARAM>(&data));

	// 3) Post WM_CLOSE for a clean shutdown (GLFW processes it via glfwPollEvents)
	if (data.hwnd != NULL)
	{
		PostMessage(data.hwnd, WM_CLOSE, 0, 0);
	}

	// 4) Clean up the process handle
	CloseHandle(processHandle);
#endif

	m_pieProcessHandle = nullptr;
	m_pieProcessId     = 0;
}

void EditorNode::CheckPIEProcessAlive()
{
	// 1) Guard: not playing
	if (!IsPlaying())
	{
		return;
	}

#ifdef _WIN32
	HANDLE processHandle = static_cast<HANDLE>(m_pieProcessHandle);

	// 2) Poll without blocking — if the process exited, clean up
	if (WaitForSingleObject(processHandle, 0) == WAIT_OBJECT_0)
	{
		CloseHandle(processHandle);
		m_pieProcessHandle = nullptr;
		m_pieProcessId     = 0;
	}
#endif
}

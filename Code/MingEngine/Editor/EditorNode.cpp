#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

namespace
{
std::string FormatNodeHandle(NodeHandle handle)
{
	return Stringf("uid=%u index=%u", handle.GetUID(), handle.GetIndex());
}
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
		DebugAddMessage(
			Stringf("Selected Node: %s", FormatNodeHandle(m_selectedNodeHandle).c_str()),
			5.f,
			Rgba8::White,
			Rgba8::White);
		return;
	}

	if (previousHandle.IsValid() && m_selectedNodeHandle.IsValid())
	{
		DebugAddMessage(
			Stringf(
				"Selection Changed: %s -> %s",
				FormatNodeHandle(previousHandle).c_str(),
				FormatNodeHandle(m_selectedNodeHandle).c_str()),
			5.f,
			Rgba8::White,
			Rgba8::White);
		return;
	}

	if (previousHandle.IsValid() && !m_selectedNodeHandle.IsValid())
	{
		DebugAddMessage("Selection Cleared", 5.f, Rgba8::White, Rgba8::White);
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

void EditorNode::SaveSceneToFile(Node const* sceneRoot, std::string const& virtualPath)
{
	Ref<PackedScene> packedScene = CreateRef<PackedScene>();

	if (!packedScene->Pack(sceneRoot))
	{
		return;
	}

	ResourceSaver::Save(virtualPath, packedScene);
}

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

	// 1) Let gizmos try first
	if (m_editorGizmos->OnBeginDrag(*camera, screenPos))
	{
		return; // gizmo ate the event
	}

	// 2) Gizmo didn't eat — try scene selection
	RaycastSpace3D*            raycastSpace = GetSceneTree()->GetRaycastSpace();
	RaycastQuery3D             raycastQuery = m_editorCamera->BuildRaycastFromMouse();
	SceneRaycastResult3D const result       = raycastSpace->IntersectRay(raycastQuery);
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
	if (g_engine->m_inputSystem->WasKeyJustPressed('1'))
	{
		SceneTree* sceneTree = GetSceneTree();
		SaveSceneToFile(sceneTree->GetScene(), "res://EditorSavedScene.mscn");
	}

	if (g_engine->m_inputSystem->WasKeyJustPressed('2'))
	{
		SceneTree*       sceneTree   = GetSceneTree();
		Ref<Resource>    loadedScene = ResourceLoader::Load("res://EditorSavedScene.mscn");
		Variant          sceneValue  = loadedScene;
		Ref<PackedScene> packedScene(sceneValue);
		Node*            newSceneRoot = packedScene.IsValid() ? packedScene->Instantiate() : nullptr;
		if (newSceneRoot != nullptr)
		{
			sceneTree->ChangeScene(newSceneRoot);
		}
	}

	if (m_editorUI != nullptr)
	{
		m_uiContext.m_isViewportImageHovered = false;

		m_editorUI->Render(m_uiContext);
	}

	// 1) Dispatch mouse events when in Pointer mode
	if (m_editorCamera != nullptr && m_editorCamera->GetControlState() == EditorCamera::EditorControlState::Pointer)
	{
		InputSystem* input     = g_engine->m_inputSystem;
		Vec2 const   cursorPos = m_editorCamera->GetCursorClientPos();
		Vec2 const   delta     = m_editorCamera->GetCursorDelta();

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

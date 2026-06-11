#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"

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

void EditorNode::SaveSceneToFile(Node const* sceneRoot, std::string const& filename)
{
	PackedScene packedScene;

	if (!packedScene.Pack(sceneRoot))
	{
		return;
	}

	std::string outputFilename = filename;
	if (outputFilename.find('.') == std::string::npos)
	{
		outputFilename += ".json";
	}

	packedScene.SaveToFile(outputFilename);
}

void EditorNode::SetActiveCamera(Camera3D* camera) { m_activeCamera = camera; }

void EditorNode::OnMouseMove(Vec2 screenPos, [[maybe_unused]] Vec2 delta)
{
	if (m_editorGizmos == nullptr || m_activeCamera == nullptr)
	{
		return;
	}

	if (m_editorGizmos->IsDragging())
	{
		m_editorGizmos->OnDrag(*m_activeCamera, screenPos);
	}
	else
	{
		m_editorGizmos->OnMouseMove(*m_activeCamera, screenPos);
	}
}

void EditorNode::OnMouseDown(int keyCode, Vec2 screenPos)
{
	if (keyCode != KeyCodeLeftMouse)
	{
		return;
	}

	if (m_editorGizmos == nullptr || m_activeCamera == nullptr)
	{
		return;
	}

	// 1. Let gizmos try first
	if (m_editorGizmos->OnBeginDrag(*m_activeCamera, screenPos))
	{
		return; // gizmo ate the event
	}

	// 2. Gizmo didn't eat — try scene selection
	NodeHandle hit = m_editorGizmos->Raycast(*m_activeCamera, screenPos);
	if (hit.IsValid())
	{
		m_selection.SetSelected(hit);
	}
	else
	{
		m_selection.Clear();
	}
}

void EditorNode::OnMouseUp(int keyCode, [[maybe_unused]] Vec2 screenPos)
{
	if (keyCode != KeyCodeLeftMouse)
	{
		return;
	}

	if (m_editorGizmos != nullptr && m_editorGizmos->IsDragging())
	{
		m_editorGizmos->OnEndDrag();
	}
}

void EditorNode::OnProcess([[maybe_unused]] float deltaSeconds)
{
	if (g_engine->m_input->WasKeyJustPressed('1'))
	{
		SceneTree* sceneTree = GetSceneTree();
		SaveSceneToFile(sceneTree->GetScene(), "EditorSavedScene");
	}

	if (g_engine->m_input->WasKeyJustPressed('2'))
	{
		SceneTree* sceneTree = GetSceneTree();

		PackedScene packedScene;
		packedScene.LoadFromFile("EditorSavedScene.json");

		Node* newSceneRoot = packedScene.Instantiate();

		if (newSceneRoot != nullptr)
		{
			sceneTree->ChangeScene(newSceneRoot);
		}
	}

	if (m_editorUI != nullptr)
	{
		EditorUIContext context;
		context.m_sceneTree = GetSceneTree();
		context.m_selection = &m_selection;
		m_editorUI->Render(context);
	}
}
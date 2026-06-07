#pragma once

#include "MingEngine/Scene/Physics/NodeRaycastUtils.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Engine/Render/Vertex.hpp"
#include "MingEngine/Engine/Math/Vec2.hpp"

#include <map>
#include <vector>

class Camera3D;

class EditorSelection
{
public:
	NodeHandle GetSelectedNodeHandle() const;
	void       SetSelected(NodeHandle handle);
	void       Clear();

private:
	NodeHandle m_selectedNodeHandle = NodeHandle::Invalid;
};

class EditorNode : public Node
{
	CLASS(EditorNode, Node);

public:
	EditorNode();
	~EditorNode() override;

	static EditorNode* Get();

	EditorSelection&       GetSelection();
	EditorSelection const& GetSelection() const;

	void RegisterSelectableMesh(std::vector<Vertex> const& verts, NodeHandle ownerHandle);
	void UnregisterSelectable(NodeHandle ownerHandle);

	void HandleSelectionClick(Camera3D const& camera, Vec2 const& clientPos);

private:
	struct EditorSelectable
	{
		NodeHandle m_ownerHandle          = NodeHandle::Invalid;
		NodeHandle m_coarseColliderHandle = NodeHandle::Invalid;
		NodeHandle m_fineColliderHandle   = NodeHandle::Invalid;
	};

	void Update(float deltaSeconds) override;

	// RenderEditorUI is giving instructions ImGui
	// which will render to the backbuffer in ImGuiSystem::EndFrame
	void RenderEditorUI();
	void RenderMainMenuBar();
	void RenderDockSpace();

	void RenderScenePanel();
	void RenderFileSystemPanel();
	void RenderViewportPanel();
	void RenderInspectorPanel();
	void RenderOutputPanel();

	GameRaycastResult Raycast(
		RaycastInfo const& info, NodeHandle& outCoarseCollider, NodeHandle& outFineCollider, NodeHandle& outSelectNode);

	void SaveSceneToFile(Node const* sceneRoot, std::string const& filename);

private:
	EditorSelection                        m_selection;
	std::map<NodeHandle, EditorSelectable> m_selectables;

	static EditorNode* s_instance;

	bool m_showScenePanel      = true;
	bool m_showFileSystemPanel = true;
	bool m_showViewportPanel   = true;
	bool m_showInspectorPanel  = true;
	bool m_showOutputPanel     = true;

	IntVec2 m_viewportPanelDimension = IntVec2::Zero;
};

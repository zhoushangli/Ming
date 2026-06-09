#pragma once

#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"
#include "MingEngine/Scene/Physics/NodeRaycastUtils.hpp"

#include "MingEngine/Engine/Math/Vec2.hpp"
#include "MingEngine/Engine/Render/Vertex.hpp"

#include <map>
#include <vector>

class Camera3D;
class EditorUI;
class EditorGizmos;

class EditorSelection
{
public:
	NodeHandle GetSelected() const;
	void       SetSelected(NodeHandle handle);
	void       Clear();

private:
	NodeHandle m_selectedNodeHandle = NodeHandle::Invalid;
};

class EditorNode : public Node
{
	MCLASS(EditorNode, Node);

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

	void OnProcess(float deltaSeconds) override;

	GameRaycastResult Raycast(
		RaycastInfo const& info, NodeHandle& outCoarseCollider, NodeHandle& outFineCollider, NodeHandle& outSelectNode);

	void SaveSceneToFile(Node const* sceneRoot, std::string const& filename);

private:
	EditorSelection                        m_selection;
	std::map<NodeHandle, EditorSelectable> m_selectables;
	EditorGizmos*                          m_editorGizmos = nullptr;
	EditorUI*                              m_editorUI     = nullptr;

	static EditorNode* s_instance;
};

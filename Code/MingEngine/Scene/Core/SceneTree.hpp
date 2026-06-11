#pragma once

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Physics/Collider3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Engine/Math/RaycastUtils.hpp"

#include <string>
#include <vector>

class Viewport;

class SceneTree
{
	friend class Node;

public:
	SceneTree();
	~SceneTree();

	// Deferred destruction:
	// 1) Nodes request destruction through QueueDestroyNode while they are in this tree.
	// 2) FlushPendingNode detaches, exits, and deletes queued nodes at a controlled point.
	// 3) The root SceneTree cannot be queued for destruction.
	void QueueDestroyNode(Node* node);
	void QueueDestroyNode(NodeHandle node);
	void FlushPendingNode();
	void QueueTransformChangedNode(NodeHandle node);
	void FlushTransformChangedNodes();

	void UpdateScene(float deltaSeconds);
	Node* GetRoot() const;
	Node* GetScene() const;
	void  ClearScene();
	void  ChangeScene(Node* newSceneNode);

	Node* ResolveNode(NodeHandle handle) const;

	void      SetUICamera(Camera3D* camera);
	Camera3D* GetWorldCamera() const;
	Camera3D* GetUICamera() const;

	virtual GameRaycastResult Raycast(RaycastInfo const& info) const;

protected:
	struct PendingReparent
	{
		NodeHandle m_newParent = NodeHandle::Invalid;
		NodeHandle m_child     = NodeHandle::Invalid;
	};

	void         RegisterNode(Node* node);
	void         UnregisterNode(Node* node);
	unsigned int FindAvailableNodeIndex() const;

	// Physics update hook:
	// 1) SceneTree calls this at a fixed interval.
	// 2) Derived scenes can override it for map-specific collision and simulation.
	virtual void UpdatePhysics(float deltaSeconds);

protected:
	std::vector<NodeHandle> m_pendingDestroyNodes;
	std::vector<NodeHandle> m_transformChangedNodes;

	std::vector<Node*> m_registeredNodes;
	unsigned int       m_nextNodeUID       = 1u;

	// tree -> root -> scene node
	Viewport*  m_root        = nullptr;
	NodeHandle m_sceneHandle = NodeHandle::Invalid;

	float m_physicsUpdateTimer    = 0.f;
	float m_physicsUpdateInterval = 1.f / 60.f;
};

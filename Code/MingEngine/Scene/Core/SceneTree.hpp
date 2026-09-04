#pragma once

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include <string>
#include <vector>

class Viewport;
class RaycastSpace3D;

class SceneTree : public Object
{
	MCLASS(SceneTree, Object);

	friend class Node;

public:
	SceneTree();
	~SceneTree() override;

	// Deferred destruction:
	// 1) Nodes request destruction through QueueDestroyNode while they are in this tree.
	// 2) FlushPendingNode detaches, exits, and deletes queued nodes at a controlled point.
	// 3) The root SceneTree cannot be queued for destruction.
	void QueueDestroyNode(Node* node);
	void FlushPendingNode();
	void QueueTransformChangedNode(ObjectID nodeID);
	void FlushTransformChangedNodes();

	void  UpdateScene(float deltaSeconds);
	Node* GetRoot() const;
	Node* GetScene() const;
	void  ClearScene();
	void  ChangeScene(Node* newSceneNode);

	void      SetUICamera(Camera3D* camera);
	Camera3D* GetWorldCamera() const;
	Camera3D* GetUICamera() const;

	RaycastSpace3D* GetRaycastSpace() const { return m_raycastSpace; }

	float GetDeltaSeconds() const;

protected:
	static void BindMethods();

	void         RegisterNode(Node* node);
	void         UnregisterNode(Node* node);
	unsigned int FindAvailableNodeIndex() const;

	// Physics update hook:
	// 1) SceneTree calls this at a fixed interval.
	// 2) Derived scenes can override it for map-specific collision and simulation.
	virtual void UpdatePhysics(float deltaSeconds);

protected:
	std::vector<ObjectID> m_pendingDestroyNodes;
	std::vector<ObjectID> m_transformChangedNodes;

	std::vector<Node*> m_registeredNodes;

	// tree -> root -> scene node
	// Scene also need pending, because the old scene needs pending to destroy safely
	Viewport* m_root         = nullptr;
	ObjectID  m_sceneID      = ObjectID::Invalid;
	Node*     m_pendingScene = nullptr;

	// Raycast space is a helping class to manage raycast objects
	// and perform raycasting in the scene.
	RaycastSpace3D* m_raycastSpace = nullptr;

	float m_deltaSeconds          = 0.f;
	float m_physicsUpdateTimer    = 0.f;
	float m_physicsUpdateInterval = 1.f / 60.f;
};

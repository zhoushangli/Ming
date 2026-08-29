#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Scene/Core/NodePath.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

#include <string>
#include <vector>

class SceneTree;
class Viewport;

// Base object for the runtime scene hierarchy.
// 1) Nodes own their children and delete them in the destructor.
// 2) Detached nodes can be built freely before being added to a SceneTree.
// 3) In-tree nodes receive lifecycle callbacks through the SceneTree traversal.
class Node : public Object
{
	MCLASS(Node, Object);

	friend class SceneTree;

public:
	enum class NotificationType : int
	{
		EnterTree = 0,
		ExitTree  = 1,
		Ready     = 2,
		Process   = 3,
		TransformChanged = 40,
	};

public:
	Node() = default;
	virtual ~Node();

	Node(Node const&)            = delete;
	Node& operator=(Node const&) = delete;

	Node*                     GetRoot() const;
	Node*                     GetParent() const;
	std::vector<Node*> const& GetChildren() const;
	std::string const&        GetName() const;
	bool                      GetSerializable() const;
	bool                      GetReady() const;
	bool                      GetProcess() const;

	void SetName(std::string const& name);
	void SetSerializable(bool isSerializable);
	void SetReady(bool isReady);
	void SetProcess(bool isProcess);

	// Currently GetNode only supports child
	// it do not support ../ or ..
	Node* FindChildByName(std::string const& name) const;
	Node* GetNode(NodePath const& path) const;

	SceneTree* GetSceneTree() const;

	// Hierarchy mutation rules:
	// 1) AddNode attaches immediately.
	// 2) DeleteNode deletes detached nodes immediately.
	// 3) DeleteNode queues in-tree nodes so SceneTree can destroy them at a safe point.
	void         AddNode(Node* child);
	void         DeleteNode();
	virtual void Reparent(Node* newParent, bool keepWorldTransform = true);

	static void BindMethods();

protected:
	void OnNotification(int notification);

	std::string EnsureUniqueName(std::string const& requestedName) const;

	// Lifecycle callbacks:
	// 1) OnEnterTree is called after this node receives a SceneTree.
	// 2) OnExitTree is called before this node unregisters from its SceneTree.
	// 3) OnReady is called after this node and its children enter a SceneTree.
	// 4) OnProcess is called once per frame by SceneTree::UpdateScene.
	
	// For c++ side, please DO NOT USE THIS, they are ONLY for script side
	// this is because these OnXXX will overrider their parent class
	// but c++ side is normally pipeline function, which we normally do not want to override
	// so for c++ side, we should use OnNotification instead, which will call along the inherit chain
	virtual void OnEnterTree();
	virtual void OnExitTree();
	virtual void OnReady();
	virtual void OnProcess(float deltaSeconds);

	// Immediate hierarchy operations:
	// 1) Used by Node and SceneTree after mutation has been validated.
	// 2) Do not call while lifecycle propagation is traversing children.
	void AttachChildImmediately(Node* child);
	void DetachChildImmediately(Node* child);
	bool IsAncestorOf(Node const* other) const;

	// 1) Sets the SceneTree for this node
	// 2) If the data already has a SceneTree, it will first call PropagateExitTree to exit the old tree.
	// 3) Call PropagateEnterTree and register this node
	void MoveToSceneTree(SceneTree* sceneTree);

	// Propagate and Notificaton
	// 1) Propagate is spread to this node and all its children of something
	// 2) Notification is sent to this node only, and will call the inherit chain in certain order
	// Process do not have propagate, because the scene tree will call process in order
	void PropagateEnterTree();
	void PropagateExitTree();
	void PropagateReady();

protected:
	struct NodeData
	{
		std::string        m_name;
		Node*              m_parent    = nullptr;
		SceneTree*         m_sceneTree = nullptr;
		Viewport*          m_viewport  = nullptr;
		std::vector<Node*> m_children;
		bool               m_isPendingDestroy = false;
		bool               m_isSerializable   = true;
		bool m_enableReady   = false;
		bool m_enableProcess = false;
	};

	NodeData m_data;
};

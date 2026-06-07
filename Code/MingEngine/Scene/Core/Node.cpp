#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"

Node::~Node()
{
	for (Node* child : m_data.m_children)
	{
		delete child;
	}
	m_data.m_children.clear();
}

Node* Node::GetRoot() const
{
	Node* current = const_cast<Node*>(this);
	while (current->m_data.m_parent != nullptr)
	{
		current = current->m_data.m_parent;
	}

	return current;
}

Node* Node::GetParent() const { return m_data.m_parent; }

SceneTree* Node::GetSceneTree() const { return m_data.m_sceneTree; }

NodeHandle Node::GetHandle() const { return m_data.m_handle; }

std::vector<Node*> const& Node::GetChildren() const { return m_data.m_children; }

std::string const& Node::GetName() const { return m_data.m_name; }

void Node::SetName(std::string const& name) { m_data.m_name = name; }

bool Node::IsSerializable() const { return m_data.m_isSerializable; }

void Node::SetSerializable(bool isSerializable) { m_data.m_isSerializable = isSerializable; }

Node* Node::FindChildByName(std::string const& name) const
{
	for (Node* child : m_data.m_children)
	{
		if (child == nullptr)
		{
			continue;
		}

		if (child->m_data.m_name == name)
		{
			return child;
		}

		Node* foundInDescendants = child->FindChildByName(name);
		if (foundInDescendants != nullptr)
		{
			return foundInDescendants;
		}
	}

	return nullptr;
}

void Node::AddNode(Node* child)
{
	if (child == nullptr || child == this || child->IsAncestorOf(this))
	{
		return;
	}

	if (m_data.m_isPendingDestroy || child->m_data.m_isPendingDestroy)
	{
		return;
	}

	SceneTree* parentSceneTree = m_data.m_sceneTree;
	SceneTree* childSceneTree  = child->m_data.m_sceneTree;

	// If child and parent are both detached， just attach immediately
	if (parentSceneTree == nullptr)
	{
		if (childSceneTree != nullptr)
		{
			ERROR_AND_DIE("Node::AddNode failed: cannot add an in-tree child to a detached parent.");
		}

		AttachChildImmediately(child);
		return;
	}

	// We do not support cross-SceneTree reparent
	// things like editor / runtime switching is basically instantiate the same file again
	if (childSceneTree != nullptr && childSceneTree != parentSceneTree)
	{
		ERROR_AND_DIE("Node::AddNode failed: cross-SceneTree reparent is not supported.");
	}

	AttachChildImmediately(child);
	child->MoveToSceneTree(parentSceneTree);
}

void Node::DeleteNode()
{
	if (m_data.m_sceneTree == nullptr)
	{
		if (m_data.m_parent != nullptr)
		{
			m_data.m_parent->DetachChildImmediately(this);
		}

		delete this;
		return;
	}

	m_data.m_sceneTree->QueueDestroyNode(this);
}

void Node::Reparent(Node* newParent, [[maybe_unused]] bool keepWorldTransform)
{
	GUARANTEE_OR_DIE(newParent != this, "Node::Reparent failed: cannot reparent a node to itself.");

	if (m_data.m_parent == newParent)
	{
		return;
	}

	newParent->AddNode(this);
}

void Node::BindMethods() {}

void Node::AttachChildImmediately(Node* child)
{
	if (child == nullptr || child == this || child->IsAncestorOf(this))
	{
		return;
	}

	if (child->m_data.m_parent != nullptr)
	{
		child->m_data.m_parent->DetachChildImmediately(child);
	}

	child->m_data.m_parent = this;
	m_data.m_children.push_back(child);
}

void Node::DetachChildImmediately(Node* child)
{
	if (child == nullptr)
	{
		return;
	}

	auto const foundChild = std::find(m_data.m_children.begin(), m_data.m_children.end(), child);
	if (foundChild == m_data.m_children.end())
	{
		return;
	}

	(*foundChild)->m_data.m_parent = nullptr;
	m_data.m_children.erase(foundChild);
}

bool Node::IsAncestorOf(Node const* other) const
{
	Node const* current = other;
	while (current != nullptr)
	{
		if (current == this)
		{
			return true;
		}
		current = current->m_data.m_parent;
	}

	return false;
}

void Node::MoveToSceneTree(SceneTree* sceneTree)
{
	if (m_data.m_sceneTree)
	{
		PropagateExitTree();
	}

	m_data.m_sceneTree = sceneTree;

	if (sceneTree != nullptr)
	{
		PropagateEnterTree();
		PropagateReady();
	}
}

void Node::PropagateEnterTree()
{
	// 1) Set tree and viewport from parent
	// sceneTree root do not have parent, so it will keep itself
	Node*              parent   = m_data.m_parent;
	std::vector<Node*> children = m_data.m_children;
	if (parent != nullptr)
	{
		m_data.m_sceneTree = parent->m_data.m_sceneTree;
	}

	// If this node is a viewport, it will keep itself
	// otherwise it will inherit from parent
	Viewport* viewport = dynamic_cast<Viewport*>(this);
	if (viewport != nullptr)
	{
		m_data.m_viewport = viewport;
	}
	else if (parent != nullptr)
	{
		m_data.m_viewport = parent->m_data.m_viewport;
	}

	// TODO: In future we should change this to object database and object handle
	m_data.m_sceneTree->RegisterNode(this);

	OnEnterTree();

	for (Node* child : children)
	{
		if (child != nullptr)
		{
			child->PropagateEnterTree();
		}
	}
}

void Node::PropagateExitTree()
{
	std::vector<Node*> children = m_data.m_children;

	for (Node* child : children)
	{
		if (child != nullptr)
		{
			child->PropagateExitTree();
		}
	}

	OnExitTree();

	m_data.m_sceneTree->UnregisterNode(this);

	m_data.m_sceneTree        = nullptr;
	m_data.m_viewport         = nullptr;
	m_data.m_isPendingDestroy = false;
}

void Node::PropagateReady()
{
	std::vector<Node*> children = m_data.m_children;

	for (Node* child : children)
	{
		if (child != nullptr)
		{
			child->PropagateReady();
		}
	}

	OnReady();
}

void Node::OnEnterTree() {}

void Node::OnExitTree() {}

void Node::OnReady() {}

void Node::Update([[maybe_unused]] float deltaSeconds) {}

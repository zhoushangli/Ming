#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <cctype>
#include <exception>

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

Node*                     Node::GetParent() const { return m_data.m_parent; }
SceneTree*                Node::GetSceneTree() const { return m_data.m_sceneTree; }
NodeHandle                Node::GetHandle() const { return m_data.m_handle; }
std::vector<Node*> const& Node::GetChildren() const { return m_data.m_children; }
std::string const&        Node::GetName() const { return m_data.m_name; }
bool                      Node::GetSerializable() const { return m_data.m_isSerializable; }
bool                      Node::GetReady() const { return m_data.m_enableReady; }
bool                      Node::GetProcess() const { return m_data.m_enableProcess; }
std::string const&        Node::GetScriptPath() const { return m_data.m_scriptPath.GetVirtualPath(); }

void Node::SetName(std::string const& name) { m_data.m_name = EnsureUniqueName(name); }
void Node::SetSerializable(bool isSerializable) { m_data.m_isSerializable = isSerializable; }
void Node::SetReady(bool isReady) { m_data.m_enableReady = isReady; }
void Node::SetProcess(bool isProcess) { m_data.m_enableProcess = isProcess; }
void Node::SetScriptPath(std::string const& scriptPath) { m_data.m_scriptPath.Parse(scriptPath); }

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

Node* Node::GetNode(NodePath const& path) const
{
	if (!path.IsValid())
	{
		return nullptr;
	}

	Node* current = path.IsAbsolute() ? GetRoot() : const_cast<Node*>(this);
	if (path.IsCurrent())
	{
		return current;
	}

	size_t                          nameIndex = 0;
	std::vector<std::string> const& paths     = path.GetPaths();
	if (path.IsAbsolute())
	{
		if (paths.empty() || paths[0] != current->GetName())
		{
			return nullptr;
		}
		nameIndex = 1;
	}

	for (; nameIndex < paths.size(); ++nameIndex)
	{
		Node* next = nullptr;
		for (Node* child : current->m_data.m_children)
		{
			if (child != nullptr && child->GetName() == paths[nameIndex])
			{
				next = child;
				break;
			}
		}

		if (next == nullptr)
		{
			return nullptr;
		}
		current = next;
	}

	return current;
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
	// If the node is not in a scene, we can delete it immediately
	if (m_data.m_sceneTree == nullptr)
	{
		if (m_data.m_parent != nullptr)
		{
			m_data.m_parent->DetachChildImmediately(this);
		}

		delete this;
		return;
	}

	// Otherwise, we need to queue the node for destruction
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

void Node::BindMethods()
{
	ClassDatabase::BindMethod("SetName", &Node::SetName);
	ClassDatabase::BindMethod("GetName", &Node::GetName);
	ClassDatabase::BindMethod("SetSerializable", &Node::SetSerializable);
	ClassDatabase::BindMethod("GetSerializable", &Node::GetSerializable);
	ClassDatabase::BindMethod("SetReady", &Node::SetReady);
	ClassDatabase::BindMethod("GetReady", &Node::GetReady);
	ClassDatabase::BindMethod("SetProcess", &Node::SetProcess);
	ClassDatabase::BindMethod("GetProcess", &Node::GetProcess);
	ClassDatabase::BindMethod("SetScriptPath", &Node::SetScriptPath);
	ClassDatabase::BindMethod("GetScriptPath", &Node::GetScriptPath);

	ADD_PROPERTY(PropertyInfo(Variant::Type::String, "name", PropertyInfo::UsageFlags::None), "SetName", "GetName");
	// ADD_PROPERTY(PropertyInfo(Variant::Type::String, "script_path", PropertyInfo::UsageFlags::Default), "SetScriptPath", "GetScriptPath");
}

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
	child->m_data.m_name = child->EnsureUniqueName(child->m_data.m_name);
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

void Node::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
	{
		OnEnterTree();
		if (m_scriptInstance != nullptr)
		{
			m_scriptInstance->CallEnterTree();
		}
		break;
	}
	case NotificationType::ExitTree:
	{
		OnExitTree();
		if (m_scriptInstance != nullptr)
		{
			m_scriptInstance->CallExitTree();
		}
		break;
	}
	case NotificationType::Ready:
	{
		OnReady();
		if (m_scriptInstance != nullptr)
		{
			m_scriptInstance->CallReady();
		}
		break;
	}
	case NotificationType::Process:
	{
		float      deltaSeconds = 0.f;
		SceneTree* sceneTree    = GetSceneTree();
		if (sceneTree != nullptr)
		{
			deltaSeconds = sceneTree->GetDeltaSeconds();
		}

		OnProcess(deltaSeconds);

		if (m_scriptInstance != nullptr)
		{
			m_scriptInstance->CallProcess(deltaSeconds);
		}
		break;
	}
	default:
		break;
	}
}

std::string Node::EnsureUniqueName(std::string const& requestedName) const
{
	std::string const normalizedName = requestedName.empty() ? GetClassName() : requestedName;
	if (m_data.m_parent == nullptr)
	{
		return normalizedName;
	}

	auto isAvailable = [this](std::string const& candidate)
	{
		for (Node const* sibling : m_data.m_parent->m_data.m_children)
		{
			if (sibling != nullptr && sibling != this && sibling->m_data.m_name == candidate)
			{
				return false;
			}
		}
		return true;
	};

	if (isAvailable(normalizedName))
	{
		return normalizedName;
	}

	size_t suffixStart = normalizedName.size();
	while (suffixStart > 0 && std::isdigit(static_cast<unsigned char>(normalizedName[suffixStart - 1])))
	{
		--suffixStart;
	}

	std::string const  baseName = normalizedName.substr(0, suffixStart);
	unsigned long long suffix   = 1;
	if (suffixStart < normalizedName.size())
	{
		try
		{
			suffix = std::stoull(normalizedName.substr(suffixStart)) + 1;
		}
		catch (std::exception const&)
		{
			suffix = 1;
		}
	}

	for (;; ++suffix)
	{
		std::string const candidate = baseName + std::to_string(suffix);
		if (isAvailable(candidate))
		{
			return candidate;
		}
	}
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

	Notification((int)NotificationType::EnterTree);

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

	Notification((int)NotificationType::ExitTree, true);

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

	if (m_data.m_enableReady)
	{
		Notification((int)NotificationType::Ready);
	}
}

void Node::OnEnterTree() {}

void Node::OnExitTree() {}

void Node::OnReady() {}

void Node::OnProcess([[maybe_unused]] float deltaSeconds) {}


#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Physics/Collider3D.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

#include "SceneTree.hpp"
#include <algorithm>

namespace
{
Vec3 GetNormalizedColor(Rgba8 const& color) { return Vec3(color.r / 255.f, color.g / 255.f, color.b / 255.f); }
} // namespace

SceneTree::SceneTree()
{
	// 1) Every SceneTree owns exactly one root Viewport.
	// 2) Entering the tree registers that Viewport with RenderService.
	m_root = new Viewport();

	m_root->MoveToSceneTree(this);
}

SceneTree::~SceneTree()
{
	if (m_root != nullptr)
	{
		// 1) Propagate exit so instances, lights, and the Viewport unregister.
		// 2) Delete the root only after all lifecycle callbacks have completed.
		m_root->PropagateExitTree();
		delete m_root;
		m_root = nullptr;
	}
}

void SceneTree::QueueDestroyNode(Node* node)
{
	if (node == nullptr || node->GetSceneTree() != this || node->m_data.m_isPendingDestroy)
	{
		return;
	}

	node->m_data.m_isPendingDestroy = true;
	QueueDestroyNode(node->GetHandle());
}

void SceneTree::QueueDestroyNode(NodeHandle handle)
{
	Node* node = ResolveNode(handle);
	if (node == nullptr)
	{
		return;
	}

	if (node->m_data.m_isPendingDestroy)
	{
		auto const foundHandle = std::find(m_pendingDestroyNodes.begin(), m_pendingDestroyNodes.end(), handle);
		if (foundHandle != m_pendingDestroyNodes.end())
		{
			return;
		}
	}
	else
	{
		node->m_data.m_isPendingDestroy = true;
	}

	m_pendingDestroyNodes.push_back(handle);
}

void SceneTree::FlushPendingNode()
{
	std::vector<NodeHandle> pendingDestroyNodes = m_pendingDestroyNodes;
	m_pendingDestroyNodes.clear();

	for (NodeHandle handle : pendingDestroyNodes)
	{
		if (handle == m_sceneHandle)
		{
			m_sceneHandle = NodeHandle::Invalid;
		}

		Node* node = ResolveNode(handle);
		if (node == nullptr || node->GetSceneTree() != this || !node->m_data.m_isPendingDestroy)
		{
			continue;
		}

		if (node->m_data.m_parent != nullptr)
		{
			node->m_data.m_parent->DetachChildImmediately(node);
		}
		node->PropagateExitTree();
		delete node;
	}
}

void SceneTree::QueueTransformChangedNode(NodeHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	if (std::find(m_transformChangedNodes.begin(), m_transformChangedNodes.end(), handle)
		!= m_transformChangedNodes.end())
	{
		return;
	}

	m_transformChangedNodes.push_back(handle);
}

void SceneTree::FlushTransformChangedNodes()
{
	std::vector<NodeHandle> transformChangedNodes = m_transformChangedNodes;
	m_transformChangedNodes.clear();

	for (NodeHandle handle : transformChangedNodes)
	{
		Node*   node   = ResolveNode(handle);
		Node3D* node3D = dynamic_cast<Node3D*>(node);
		if (node3D != nullptr)
		{
			node3D->OnTransformChanged();
		}
	}
}

void SceneTree::UpdateScene(float deltaSeconds)
{
	m_physicsUpdateTimer += deltaSeconds;
	while (m_physicsUpdateTimer >= m_physicsUpdateInterval)
	{
		UpdatePhysics(m_physicsUpdateInterval);
		m_physicsUpdateTimer -= m_physicsUpdateInterval;
	}

	std::vector<Node*> registeredNodes = m_registeredNodes;
	for (Node* node : registeredNodes)
	{
		if (node == nullptr)
		{
			continue;
		}

		if (node->m_data.m_enableProcess)
		{
			node->OnProcess(deltaSeconds);
		}
	}

	UpdatePhysics(deltaSeconds);
	FlushTransformChangedNodes();
}

Node* SceneTree::GetRoot() const { return m_root; }

Node* SceneTree::GetScene() const { return ResolveNode(m_sceneHandle); }

void SceneTree::ClearScene()
{
	Node* sceneRoot = GetScene();
	m_sceneHandle   = NodeHandle::Invalid;
	if (sceneRoot == nullptr)
	{
		return;
	}

	sceneRoot->DeleteNode();
}

void SceneTree::ChangeScene(Node* newSceneNode)
{
	ClearScene();

	m_root->AddNode(newSceneNode);
	m_sceneHandle = newSceneNode->GetHandle();
}

Camera3D* SceneTree::GetWorldCamera() const { return m_root->GetWorldCamera(); }

GameRaycastResult SceneTree::Raycast(RaycastInfo const& info) const
{
	GameRaycastResult result;
	result.m_rayStartPos  = info.m_startPos;
	result.m_rayFwdNormal = info.m_forwardNormal;
	result.m_rayMaxLength = info.m_maxLength;
	return result;
}

Node* SceneTree::ResolveNode(NodeHandle handle) const
{
	if (!handle.IsValid())
	{
		return nullptr;
	}

	unsigned int const index = handle.GetIndex();
	if (index >= m_registeredNodes.size())
	{
		return nullptr;
	}

	Node* node = m_registeredNodes[index];
	if (node == nullptr || node->m_data.m_handle != handle)
	{
		return nullptr;
	}

	return node;
}

void SceneTree::RegisterNode(Node* node)
{
	if (node == nullptr)
	{
		return;
	}

	if (node->m_data.m_handle.IsValid() && ResolveNode(node->m_data.m_handle) == node)
	{
		return;
	}

	unsigned int const index = FindAvailableNodeIndex();
	if (index == m_registeredNodes.size())
	{
		m_registeredNodes.push_back(node);
	}
	else
	{
		m_registeredNodes[index] = node;
	}

	unsigned int uid = m_nextNodeUID & 0x0000ffffu;
	if (uid == 0x0000ffffu)
	{
		uid = 1u;
	}
	++m_nextNodeUID;

	node->m_data.m_handle = NodeHandle(uid, index);
}

void SceneTree::UnregisterNode(Node* node)
{
	if (node == nullptr || !node->m_data.m_handle.IsValid())
	{
		return;
	}

	unsigned int const index = node->m_data.m_handle.GetIndex();
	if (index < m_registeredNodes.size() && m_registeredNodes[index] == node)
	{
		m_registeredNodes[index] = nullptr;
	}

	node->m_data.m_handle = NodeHandle::Invalid;
}

unsigned int SceneTree::FindAvailableNodeIndex() const
{
	for (unsigned int index = 0; index < (unsigned int)m_registeredNodes.size(); ++index)
	{
		if (m_registeredNodes[index] == nullptr)
		{
			return index;
		}
	}

	return (unsigned int)m_registeredNodes.size();
}

void SceneTree::UpdatePhysics([[maybe_unused]] float deltaSeconds) {

};

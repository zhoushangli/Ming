#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include <algorithm>

namespace
{
Vector3 GetNormalizedColor(Color const& color) { return Vector3(color.r / 255.f, color.g / 255.f, color.b / 255.f); }
} // namespace

SceneTree::SceneTree()
{
	// raycast space should create first and delete last, so node can enter / exit it
	m_raycastSpace = new RaycastSpace3D();

	// 1) Every SceneTree owns exactly one root Viewport.
	// 2) Entering the tree registers that Viewport with RenderService.
	m_root = new Viewport();
	m_root->SetName("Root");

	m_root->MoveToSceneTree(this);
}

SceneTree::~SceneTree()
{
	if (m_root != nullptr)
	{
		// 1) Propagate exit so instances, lights, and the Viewport unregister.
		// 2) Delete the root only after all lifecycle callbacks have completed.
		m_root->MoveToSceneTree(nullptr);
		delete m_root;
		m_root = nullptr;
	}

	if (m_raycastSpace != nullptr)
	{
		delete m_raycastSpace;
		m_raycastSpace = nullptr;
	}
}

void SceneTree::QueueDestroyNode(Node* node)
{
	if (node == nullptr || node->GetSceneTree() != this || node->m_data.m_isPendingDestroy)
	{
		return;
	}

	ObjectID const nodeID = node->GetObjectID();
	if (!nodeID.IsValid())
	{
		return;
	}

	node->m_data.m_isPendingDestroy = true;
	m_pendingDestroyNodes.push_back(nodeID);
}

void SceneTree::FlushPendingNode()
{
	// Flush pending scene
	if (m_pendingScene != nullptr)
	{
		Node* previousScene = ObjectDatabase::GetInstance<Node>(m_sceneID);
		if (previousScene != nullptr && previousScene->GetSceneTree() == this)
		{
			m_root->DetachChildImmediately(previousScene);
			previousScene->MoveToSceneTree(nullptr);
			delete previousScene;
		}

		m_root->AddNode(m_pendingScene);
		m_sceneID      = m_pendingScene->GetObjectID();
		m_pendingScene = nullptr;
	}

	// Flush pending destroy nodes
	std::vector<ObjectID> pendingDestroyNodes = m_pendingDestroyNodes;
	m_pendingDestroyNodes.clear();

	for (ObjectID nodeID : pendingDestroyNodes)
	{
		if (nodeID == m_sceneID)
		{
			m_sceneID = ObjectID::Invalid;
		}

		Node* node = ObjectDatabase::GetInstance<Node>(nodeID);
		if (node == nullptr || node->GetSceneTree() != this || !node->m_data.m_isPendingDestroy)
		{
			continue;
		}

		if (node->m_data.m_parent != nullptr)
		{
			node->m_data.m_parent->DetachChildImmediately(node);
		}
		node->MoveToSceneTree(nullptr);
		delete node;
	}
}

void SceneTree::QueueTransformChangedNode(ObjectID nodeID)
{
	if (!nodeID.IsValid())
	{
		return;
	}

	if (std::find(m_transformChangedNodes.begin(), m_transformChangedNodes.end(), nodeID)
		!= m_transformChangedNodes.end())
	{
		return;
	}

	m_transformChangedNodes.push_back(nodeID);
}

void SceneTree::FlushTransformChangedNodes()
{
	std::vector<ObjectID> transformChangedNodes = m_transformChangedNodes;
	m_transformChangedNodes.clear();

	for (ObjectID nodeID : transformChangedNodes)
	{
		Node3D* node3D = ObjectDatabase::GetInstance<Node3D>(nodeID);
		if (node3D != nullptr && node3D->GetSceneTree() == this)
		{
			node3D->OnTransformChanged();
		}
	}
}

void SceneTree::UpdateScene(float deltaSeconds)
{
	m_deltaSeconds = deltaSeconds;
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
			node->Notification((int)Node::NotificationType::Process);
		}
	}

	UpdatePhysics(deltaSeconds);
	FlushTransformChangedNodes();
}

Node* SceneTree::GetRoot() const { return m_root; }

Node* SceneTree::GetScene() const
{
	Node* scene = ObjectDatabase::GetInstance<Node>(m_sceneID);
	return scene != nullptr && scene->GetSceneTree() == this ? scene : nullptr;
}

void SceneTree::ClearScene()
{
	Node* sceneRoot = GetScene();
	if (sceneRoot == nullptr)
	{
		return;
	}

	sceneRoot->DeleteNode();
}

void SceneTree::ChangeScene(Node* newSceneNode)
{
	ClearScene();

	m_pendingScene = newSceneNode;
}

Camera3D* SceneTree::GetWorldCamera() const { return m_root->GetWorldCamera(); }

float SceneTree::GetDeltaSeconds() const { return m_deltaSeconds; }

void SceneTree::RegisterNode(Node* node)
{
	if (node == nullptr)
	{
		return;
	}

	if (std::find(m_registeredNodes.begin(), m_registeredNodes.end(), node) != m_registeredNodes.end())
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
}

void SceneTree::UnregisterNode(Node* node)
{
	if (node == nullptr)
	{
		return;
	}

	auto const foundNode = std::find(m_registeredNodes.begin(), m_registeredNodes.end(), node);
	if (foundNode != m_registeredNodes.end())
	{
		*foundNode = nullptr;
	}
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

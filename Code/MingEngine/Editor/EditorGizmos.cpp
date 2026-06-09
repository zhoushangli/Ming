#include "MingEngine/Editor/EditorGizmos.hpp"

#include "EditorGizmos.hpp"
#include "MingEngine/Editor/GizmosShapes.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

void EditorGizmos::OnReady()
{
	Node::OnReady();
	SetWorldGridVisible(true);
	SetWorldAxisVisible(true);
}

EditorGizmos::EditorGizmos()
{
	SetReady(true);
	SetProcess(true);
}

EditorGizmos::~EditorGizmos() {}

void EditorGizmos::SetWorldGridVisible(bool visible)
{
	SceneTree* sceneTree = GetSceneTree();
	Node*      worldGrid = sceneTree != nullptr ? sceneTree->ResolveNode(m_worldGridHandle) : nullptr;

	if (visible)
	{
		if (worldGrid == nullptr)
		{
			EditorWorldGrid3D* newWorldGrid = new EditorWorldGrid3D();
			newWorldGrid->SetName("WorldGrid");
			newWorldGrid->SetSerializable(false);
			AddNode(newWorldGrid);
			m_worldGridHandle = newWorldGrid->GetHandle();
		}
		return;
	}

	if (worldGrid != nullptr)
	{
		worldGrid->DeleteNode();
	}
	m_worldGridHandle = NodeHandle::Invalid;
}

void EditorGizmos::SetWorldAxisVisible(bool visible)
{
	SceneTree* sceneTree = GetSceneTree();
	Node*      worldAxis = sceneTree != nullptr ? sceneTree->ResolveNode(m_worldAxisHandle) : nullptr;

	if (visible)
	{
		if (worldAxis == nullptr)
		{
			EditorWorldAxis3D* newWorldAxis = new EditorWorldAxis3D();
			newWorldAxis->SetName("WorldAxis");
			newWorldAxis->SetSerializable(false);
			AddNode(newWorldAxis);
			m_worldAxisHandle = newWorldAxis->GetHandle();
		}
		return;
	}

	if (worldAxis != nullptr)
	{
		worldAxis->DeleteNode();
	}
	m_worldAxisHandle = NodeHandle::Invalid;
}

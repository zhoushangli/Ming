#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"

#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"
#include "MingEngine/Editor/Gizmos/EditorNodeGizmo3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include <set>
#include <functional>
#include "MingEngine/Editor/Gizmos/ViewportAxisIndicator.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"

using namespace Math;

namespace
{
float constexpr kEditorGizmoRaycastLength = 10000.f;
float constexpr kWorldAxisExtent          = 100.f;

Color const kAxisXColor(255, 70, 105, 255);
Color const kAxisYColor(155, 225, 20, 255);
Color const kAxisZColor(55, 160, 255, 255);

EditorWorldAxis3D* CreateWorldAxis(
	Node* parent, char const* name, Vector3 const& axisStart, Vector3 const& axisEnd, Color const& color)
{
	EditorWorldAxis3D* axis = MemNew<EditorWorldAxis3D>(axisStart, axisEnd, color);
	axis->SetName(name);
	axis->SetSerializable(false);
	parent->AddNode(axis);
	return axis;
}
} // namespace

EditorGizmos::EditorGizmos()
{
	SetReady(true);
	SetProcess(true);

	m_worldGrid = MemNew<EditorWorldGrid3D>();
	m_worldGrid->SetName("WorldGrid");
	m_worldGrid->SetSerializable(false);
	AddNode(m_worldGrid);

	m_worldAxises[0] =
		CreateWorldAxis(this, "WorldAxisXPositive", Vector3::Zero, Vector3(kWorldAxisExtent, 0.f, 0.f), kAxisXColor);
	m_worldAxises[1] =
		CreateWorldAxis(this, "WorldAxisXNegative", Vector3::Zero, Vector3(-kWorldAxisExtent, 0.f, 0.f), kAxisXColor);
	m_worldAxises[2] =
		CreateWorldAxis(this, "WorldAxisYPositive", Vector3::Zero, Vector3(0.f, kWorldAxisExtent, 0.f), kAxisYColor);
	m_worldAxises[3] =
		CreateWorldAxis(this, "WorldAxisYNegative", Vector3::Zero, Vector3(0.f, -kWorldAxisExtent, 0.f), kAxisYColor);
	m_worldAxises[4] =
		CreateWorldAxis(this, "WorldAxisZPositive", Vector3::Zero, Vector3(0.f, 0.f, kWorldAxisExtent), kAxisZColor);
	m_worldAxises[5] =
		CreateWorldAxis(this, "WorldAxisZNegative", Vector3::Zero, Vector3(0.f, 0.f, -kWorldAxisExtent), kAxisZColor);

	m_transformGizmo = MemNew<TransformGizmo3D>();
	m_transformGizmo->SetName("TransformGizmo3D");
	m_transformGizmo->SetSerializable(false);
	AddNode(m_transformGizmo);

	m_axisIndicator = MemNew<ViewportAxisIndicator>();
	m_axisIndicator->SetName("ViewportAxisIndicator");
	m_axisIndicator->SetSerializable(false);
	AddNode(m_axisIndicator);
}

EditorGizmos::~EditorGizmos()
{
}

void EditorGizmos::OnMouseMove(Camera3D const& camera, Vector2 screenPos)
{
	GizmoContext const ctx = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	SyncNodeGizmos(ctx);
	m_nodeHovered = false;
	for (auto const& entry : m_nodeGizmos)
	{
		float     distance = 10000.f;
		int const handle   = entry.second->HitTest(ctx, distance, true);
		entry.second->SetHoveredHandle(handle);
		m_nodeHovered = m_nodeHovered || handle >= 0;
	}
	if (m_nodeHovered)
	{
		m_transformGizmo->ClearHover();
	}
	else
	{
		m_transformGizmo->UpdateHover(ctx);
	}
}

bool EditorGizmos::BeginDragHovered(Camera3D const& camera, Vector2 screenPos)
{
	if (IsDragging())
	{
		return false;
	}

	GizmoContext const ctx = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	SyncNodeGizmos(ctx);
	for (auto const& entry : m_nodeGizmos)
	{
		float     distance = 10000.f;
		int const handle   = entry.second->HitTest(ctx, distance, true);
		if (handle >= 0 && entry.second->BeginHandleDrag(handle, ctx))
		{
			m_activeNodeGizmo = entry.second;
			m_transformGizmo->ClearHover();
			return true;
		}
	}
	return m_transformGizmo->BeginDragHovered(ctx);
}

void EditorGizmos::OnDrag(Camera3D const& camera, Vector2 screenPos)
{
	GizmoContext const ctx = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	if (m_activeNodeGizmo != nullptr)
	{
		m_activeNodeGizmo->UpdateHandleDrag(ctx);
		if (!m_activeNodeGizmo->IsDragging())
		{
			m_activeNodeGizmo = nullptr;
		}
	}
	else
	{
		m_transformGizmo->OnDrag(ctx);
	}
}

void EditorGizmos::OnEndDrag()
{
	if (m_activeNodeGizmo != nullptr)
	{
		m_activeNodeGizmo->EndHandleDrag(true);
		m_activeNodeGizmo = nullptr;
		ClearHover();
		return;
	}

	if (m_transformGizmo->IsDragging())
	{
		Camera3D* camera = EditorCamera::Get() ? EditorCamera::Get()->GetCamera() : nullptr;
		if (camera != nullptr)
		{
			Vector2 const      cursorPos = Vector2(g_engine->m_inputSystem->GetCursorClientPosition());
			GizmoContext const ctx       = BuildGizmoContext(GetSceneTree(), *camera, cursorPos);
			m_transformGizmo->EndDrag(ctx);
		}
	}
}

bool EditorGizmos::IsHovered() const
{
	return m_nodeHovered || (m_transformGizmo != nullptr && m_transformGizmo->IsHovered());
}

bool EditorGizmos::IsDragging() const
{
	return (m_activeNodeGizmo != nullptr && m_activeNodeGizmo->IsDragging())
		   || (m_transformGizmo != nullptr && m_transformGizmo->IsDragging());
}

void EditorGizmos::CancelDrag()
{
	if (m_activeNodeGizmo != nullptr)
	{
		m_activeNodeGizmo->EndHandleDrag(false);
		m_activeNodeGizmo = nullptr;
	}

	m_transformGizmo->CancelDrag();
	ClearHover();
}

void EditorGizmos::ClearHover()
{
	if (!IsDragging())
	{
		m_transformGizmo->ClearHover();
		m_nodeHovered = false;
		for (auto const& entry : m_nodeGizmos)
		{
			entry.second->SetHoveredHandle(-1);
		}
	}
}

void EditorGizmos::SetTool(GizmoTool tool)
{
	if (!IsDragging())
	{
		m_tool = tool;
		ClearHover();
	}
}

void EditorGizmos::SetLocal(bool local)
{
	if (!IsDragging())
	{
		m_local = local;
		ClearHover();
	}
}

void EditorGizmos::SetGridVisible(bool visible)
{
	m_worldGrid->SetVisible(visible);
}

bool EditorGizmos::IsGridVisible() const
{
	return m_worldGrid->GetVisible();
}

void EditorGizmos::SyncNodeGizmos(GizmoContext const& context)
{
	std::set<ObjectID>         alive;
	Node*                      scene = GetSceneTree() != nullptr ? GetSceneTree()->GetScene() : nullptr;
	std::function<void(Node*)> visit = [&](Node* node)
	{
		if (node == nullptr || node->IsQueuedForDeletion() || !node->GetSerializable())
		{
			return;
		}
		auto* target = dynamic_cast<Node3D*>(node);
		if (target != nullptr
			&& (dynamic_cast<Light3D*>(target) != nullptr || dynamic_cast<Camera3D*>(target) != nullptr))
		{
			ObjectID const id = target->GetObjectID();
			alive.insert(id);
			if (m_nodeGizmos.find(id) == m_nodeGizmos.end())
			{
				EditorNodeGizmo3D* gizmo = dynamic_cast<Light3D*>(target) != nullptr
											   ? static_cast<EditorNodeGizmo3D*>(MemNew<Light3DGizmo>(target))
											   : static_cast<EditorNodeGizmo3D*>(MemNew<Camera3DGizmo>(target));
				AddNode(gizmo);
				m_nodeGizmos.emplace(id, gizmo);
			}
		}
		for (Node* child : node->GetChildren())
		{
			visit(child);
		}
	};
	visit(scene);

	for (auto it = m_nodeGizmos.begin(); it != m_nodeGizmos.end();)
	{
		if (alive.find(it->first) == alive.end())
		{
			if (m_activeNodeGizmo == it->second)
			{
				CancelDrag();
			}
			it->second->SetVisible(false);
			it->second->QueueFree();
			it = m_nodeGizmos.erase(it);
		}
		else
		{
			it->second->UpdateView(context);
			++it;
		}
	}
}

ObjectID EditorGizmos::PickNode(Camera3D const& camera, Vector2 screenPos, float maxDistance)
{
	GizmoContext const context = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	SyncNodeGizmos(context);
	ObjectID result = ObjectID::Invalid;
	for (auto const& entry : m_nodeGizmos)
	{
		float distance = maxDistance;
		if (entry.second->HitTest(context, distance) != -1 && distance < maxDistance)
		{
			result      = entry.first;
			maxDistance = distance;
		}
	}
	return result;
}

void EditorGizmos::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Process:
	{
		auto* editorCamera = EditorCamera::Get();
		auto* camera       = editorCamera != nullptr ? editorCamera->GetCamera() : nullptr;
		if (camera != nullptr)
		{
			SyncNodeGizmos(BuildGizmoContext(GetSceneTree(), *camera));
		}
		if (m_activeNodeGizmo != nullptr && !m_activeNodeGizmo->IsDragging())
		{
			m_activeNodeGizmo = nullptr;
		}
		break;
	}
	case Notification_ExitTree:
	case Notification_PreDelete:
		CancelDrag();
		break;
	}
}

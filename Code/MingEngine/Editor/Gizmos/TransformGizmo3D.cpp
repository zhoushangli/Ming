#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/GizmoRaycastObject.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Physics/NodeRaycastUtils.hpp"

using namespace Math;

namespace
{
Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);

float constexpr kEditorGizmoRaycastLength = 10000.f;

RaycastQuery3D BuildRaycastQuery(GizmoContext const& context)
{
	RaycastQuery3D query;
	query.m_maxDistance = kEditorGizmoRaycastLength;

	if (context.m_camera != nullptr)
	{
		float const   aspect = context.m_clientDimensions.x / Max(context.m_clientDimensions.y, 1.f);
		CameraContext camCtx = context.m_camera->GetCameraContext(aspect);
		RaycastInfo   info =
			BuildRaycastFromMouse(camCtx, context.m_clientPos, context.m_clientDimensions, kEditorGizmoRaycastLength);

		query.m_start     = info.m_startPos;
		query.m_direction = info.m_forwardNormal;
		query.m_exclude   = info.m_ignoreNodeHandle;
	}

	return query;
}
} // namespace

TransformGizmo3D::TransformGizmo3D()
{
	SetReady(true);
	SetProcess(true);

	GizmoAxisArrow*   arrowX = new GizmoAxisArrow(GizmoAxis::X, kAxisXColor);
	GizmoAxisArrow*   arrowY = new GizmoAxisArrow(GizmoAxis::Y, kAxisYColor);
	GizmoAxisArrow*   arrowZ = new GizmoAxisArrow(GizmoAxis::Z, kAxisZColor);
	GizmoPlaneSquare* planeX = new GizmoPlaneSquare(GizmoAxis::X, kAxisXColor);
	GizmoPlaneSquare* planeY = new GizmoPlaneSquare(GizmoAxis::Y, kAxisYColor);
	GizmoPlaneSquare* planeZ = new GizmoPlaneSquare(GizmoAxis::Z, kAxisZColor);
	GizmoRotationArc* arcX   = new GizmoRotationArc(GizmoAxis::X, kAxisXColor);
	GizmoRotationArc* arcY   = new GizmoRotationArc(GizmoAxis::Y, kAxisYColor);
	GizmoRotationArc* arcZ   = new GizmoRotationArc(GizmoAxis::Z, kAxisZColor);

	arrowX->SetSerializable(false);
	arrowY->SetSerializable(false);
	arrowZ->SetSerializable(false);
	planeX->SetSerializable(false);
	planeY->SetSerializable(false);
	planeZ->SetSerializable(false);
	arcX->SetSerializable(false);
	arcY->SetSerializable(false);
	arcZ->SetSerializable(false);

	AddNode(arrowX);
	AddNode(arrowY);
	AddNode(arrowZ);
	AddNode(planeX);
	AddNode(planeY);
	AddNode(planeZ);
	AddNode(arcX);
	AddNode(arcY);
	AddNode(arcZ);

	m_components.push_back(arrowX);
	m_components.push_back(arrowY);
	m_components.push_back(arrowZ);
	m_components.push_back(planeX);
	m_components.push_back(planeY);
	m_components.push_back(planeZ);
	m_components.push_back(arcX);
	m_components.push_back(arcY);
	m_components.push_back(arcZ);
}

TransformGizmo3D::~TransformGizmo3D() { m_components.clear(); }

void TransformGizmo3D::UpdateHover(GizmoContext const& context)
{
	if (m_activeComponent != nullptr)
	{
		return;
	}

	GizmoComponent* hitComponent = HitTest(context);
	m_hoveredHitPos              = context.m_originWorld;
	if (hitComponent != nullptr)
	{
		RaycastQuery3D  query  = BuildRaycastQuery(context);
		RaycastSpace3D* space  = context.m_sceneTree != nullptr ? context.m_sceneTree->GetRaycastSpace() : nullptr;
		RaycastResult3D result = space != nullptr ? space->IntersectRay(query) : RaycastResult3D();
		if (result.m_didImpact)
		{
			m_hoveredHitPos = result.m_impactPos;
		}
	}

	if (m_hoveredComponent != hitComponent)
	{
		if (m_hoveredComponent != nullptr)
		{
			m_hoveredComponent->SetHovered(false);
		}

		m_hoveredComponent = hitComponent;
		if (m_hoveredComponent != nullptr)
		{
			m_hoveredComponent->SetHovered(true);
		}
	}
}

bool TransformGizmo3D::BeginDragHovered(GizmoContext const& context)
{
	if (m_hoveredComponent == nullptr)
	{
		return false;
	}

	m_hoveredComponent->SetHovered(true);
	m_activeComponent = m_hoveredComponent;
	if (context.m_selectedNode3D != nullptr)
	{
		m_draggedNodeHandle    = context.m_selectedNode3D->GetHandle();
		m_dragStartPosition    = context.m_selectedNode3D->GetLocalPosition();
		m_dragStartOrientation = context.m_selectedNode3D->GetLocalOrientation();
		m_dragStartScale       = context.m_selectedNode3D->GetLocalScale();
	}

	m_activeComponent->OnBeginDrag(context, m_hoveredHitPos);
	return true;
}

void TransformGizmo3D::OnDrag(GizmoContext const& context)
{
	if (m_activeComponent != nullptr)
	{
		RaycastQuery3D query    = BuildRaycastQuery(context);
		Vec3           rayStart = query.m_start;
		Vec3           rayFwd   = query.m_direction;
		m_activeComponent->OnDrag(context, rayStart, rayFwd);
	}
}

void TransformGizmo3D::EndDrag(GizmoContext const& context)
{
	if (m_activeComponent != nullptr)
	{
		m_activeComponent->OnEndDrag(context);
		Node3D*    draggedNode = context.m_sceneTree != nullptr
									 ? dynamic_cast<Node3D*>(context.m_sceneTree->ResolveNode(m_draggedNodeHandle))
									 : nullptr;
		bool const changed     = draggedNode != nullptr
								 && (draggedNode->GetLocalPosition() != m_dragStartPosition
									 || draggedNode->GetLocalOrientation() != m_dragStartOrientation
									 || draggedNode->GetLocalScale() != m_dragStartScale);
		if (changed && EditorNode::Get() != nullptr)
		{
			EditorNode::Get()->MarkSceneDirty();
		}
		m_draggedNodeHandle = NodeHandle::Invalid;
		m_activeComponent   = nullptr;
	}
}

void TransformGizmo3D::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::Process:
	{
		EditorCamera* editorCamera = EditorCamera::Get();
		Camera3D*     camera       = editorCamera != nullptr ? editorCamera->GetCamera() : nullptr;
		if (camera == nullptr)
		{
			for (GizmoComponent* component : m_components)
			{
				component->SetVisible(false);
			}
			return;
		}

		GizmoContext const context        = BuildGizmoContext(GetSceneTree(), *camera);
		bool const         hasSelection   = context.m_selectedNode3D != nullptr;
		bool const         rotationActive = m_activeComponent != nullptr && m_activeComponent->IsRotationGizmo();

		for (GizmoComponent* component : m_components)
		{
			bool const isVisible = hasSelection && (!rotationActive || component == m_activeComponent);
			component->SetVisible(isVisible);
			if (isVisible)
			{
				component->SetWorldPosition(context.m_originWorld);
				component->SetWorldScale(Vec3(context.m_scale, context.m_scale, context.m_scale));
				component->UpdateRaycastObject(context);
			}
		}

		Vec3 const cameraPos = camera->GetWorldPosition();
		std::stable_sort(
			m_components.begin(),
			m_components.end(),
			[&](GizmoComponent* a, GizmoComponent* b)
			{
				float const distA = (a->GetWorldVirtualCenter() - cameraPos).GetLengthSquared();
				float const distB = (b->GetWorldVirtualCenter() - cameraPos).GetLengthSquared();
				return distA > distB;
			});

		int constexpr kGizmoPriorityBase = 1000;
		for (size_t rank = 0; rank < m_components.size(); ++rank)
		{
			m_components[rank]->SetRenderPriority(kGizmoPriorityBase + static_cast<int>(rank));
			// DebugGizmos::AddWorldSphere(
			// 	m_components[rank]->GetWorldVirtualCenter(),
			// 	0.1f,
			// 	0.f,
			// 	Rgba8(255, 255, 0, 255),
			// 	Rgba8(255, 255, 0, 255),
			// 	DebugRenderMode::X_RAY);
		}

		break;
	}
	}
}

bool TransformGizmo3D::IsHovered() const { return m_hoveredComponent != nullptr; }

bool TransformGizmo3D::IsDragging() const { return m_activeComponent != nullptr; }

GizmoComponent* TransformGizmo3D::HitTest(GizmoContext const& context) const
{
	if (context.m_selectedNode3D == nullptr || context.m_sceneTree == nullptr)
	{
		return nullptr;
	}

	RaycastSpace3D* space = context.m_sceneTree->GetRaycastSpace();
	if (space == nullptr)
	{
		return nullptr;
	}

	RaycastQuery3D  query  = BuildRaycastQuery(context);
	RaycastResult3D result = space->IntersectRay(query);
	if (!result.m_didImpact)
	{
		return nullptr;
	}

	// Resolve the hit node handle to a GizmoComponent
	Node* node = context.m_sceneTree->ResolveNode(result.m_owner);
	return dynamic_cast<GizmoComponent*>(node);
}

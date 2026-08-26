#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"

#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

namespace
{
Color const     kAxisXColor(255, 70, 105, 255);
Color const     kAxisYColor(155, 225, 20, 255);
Color const     kAxisZColor(55, 160, 255, 255);
constexpr float kRaycastMaxLength = 10000.f;

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

	m_hoveredHitPos              = context.m_originWorld;
	GizmoComponent* hitComponent = HitTest(context, m_hoveredHitPos);

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
	if (context.m_camera == nullptr)
	{
		return false;
	}

	m_dragStartRaycastInfo =
		context.m_camera->BuildRaycastFromMouse(context.m_clientPos, context.m_clientDimensions, kRaycastMaxLength);
	m_hoveredComponent->SetHovered(true);
	m_activeComponent = m_hoveredComponent;
	if (context.m_selectedNode3D != nullptr)
	{
		m_draggedNodeID        = context.m_selectedNode3D->GetObjectID();
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
		if (context.m_camera == nullptr)
		{
			return;
		}

		MathRaycastQuery3D const currentRaycastInfo =
			context.m_camera->BuildRaycastFromMouse(context.m_clientPos, context.m_clientDimensions, kRaycastMaxLength);
		m_activeComponent->OnDrag(context, m_dragStartRaycastInfo, currentRaycastInfo);
	}
}

void TransformGizmo3D::EndDrag(GizmoContext const& context)
{
	if (m_activeComponent != nullptr)
	{
		m_activeComponent->OnEndDrag(context);
		Node3D* draggedNode = ObjectDatabase::GetInstance<Node3D>(m_draggedNodeID);
		if (context.m_sceneTree == nullptr
			|| (draggedNode != nullptr && draggedNode->GetSceneTree() != context.m_sceneTree))
		{
			draggedNode = nullptr;
		}
		bool const changed     = draggedNode != nullptr
								 && (draggedNode->GetLocalPosition() != m_dragStartPosition
									 || draggedNode->GetLocalOrientation() != m_dragStartOrientation
									 || draggedNode->GetLocalScale() != m_dragStartScale);
		if (changed && EditorNode::Get() != nullptr)
		{
			EditorNode::Get()->MarkSceneDirty();
		}
		m_draggedNodeID     = ObjectID::Invalid;
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
				component->SetWorldScale(Vector3(context.m_scale, context.m_scale, context.m_scale));
			}
		}

		Vector3 const cameraPos = camera->GetWorldPosition();
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
		}

		break;
	}
	}
}

bool TransformGizmo3D::IsHovered() const { return m_hoveredComponent != nullptr; }

bool TransformGizmo3D::IsDragging() const { return m_activeComponent != nullptr; }

GizmoComponent* TransformGizmo3D::HitTest(GizmoContext const& context, Vector3& outHitPos) const
{
	if (context.m_selectedNode3D == nullptr || context.m_camera == nullptr)
	{
		return nullptr;
	}

	MathRaycastQuery3D const raycastInfo =
		context.m_camera->BuildRaycastFromMouse(context.m_clientPos, context.m_clientDimensions, kRaycastMaxLength);
	GizmoComponent* closestComponent = nullptr;
	float           closestDistance  = raycastInfo.m_maxLength;
	for (GizmoComponent* component : m_components)
	{
		if (component == nullptr || !component->GetVisible())
		{
			continue;
		}

		MathRaycastResult3D const hit = component->Raycast(context, raycastInfo);
		if (hit.m_didImpact && hit.m_impactDist < closestDistance)
		{
			closestComponent = component;
			closestDistance  = hit.m_impactDist;
			outHitPos        = hit.m_impactPos;
		}
	}

	return closestComponent;
}

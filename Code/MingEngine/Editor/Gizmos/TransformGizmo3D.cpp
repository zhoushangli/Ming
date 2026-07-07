#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"

#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

namespace
{
Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);
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
	// GizmoRotationArc* arcX   = new GizmoRotationArc(GizmoAxis::X, kAxisXColor);
	// GizmoRotationArc* arcY   = new GizmoRotationArc(GizmoAxis::Y, kAxisYColor);
	// GizmoRotationArc* arcZ   = new GizmoRotationArc(GizmoAxis::Z, kAxisZColor);

	arrowX->SetSerializable(false);
	arrowY->SetSerializable(false);
	arrowZ->SetSerializable(false);
	planeX->SetSerializable(false);
	planeY->SetSerializable(false);
	planeZ->SetSerializable(false);
	// arcX->SetSerializable(false);
	// arcY->SetSerializable(false);
	// arcZ->SetSerializable(false);

	AddNode(arrowX);
	AddNode(arrowY);
	AddNode(arrowZ);
	AddNode(planeX);
	AddNode(planeY);
	AddNode(planeZ);
	// AddNode(arcX);
	// AddNode(arcY);
	// AddNode(arcZ);

	m_components.push_back(arrowX);
	m_components.push_back(arrowY);
	m_components.push_back(arrowZ);
	m_components.push_back(planeX);
	m_components.push_back(planeY);
	m_components.push_back(planeZ);
	// m_components.push_back(arcX);
	// m_components.push_back(arcY);
	// m_components.push_back(arcZ);
}

TransformGizmo3D::~TransformGizmo3D() { m_components.clear(); }

void TransformGizmo3D::UpdateHover(GizmoContext const& context, RaycastInfo const& ray)
{
	if (m_activeComponent != nullptr)
	{
		return;
	}

	GizmoRaycastResult const result = Raycast(context, ray);
	if (m_hoveredComponent != result.m_component)
	{
		if (m_hoveredComponent != nullptr)
		{
			m_hoveredComponent->SetHovered(false);
		}

		m_hoveredComponent = result.m_component;
		if (m_hoveredComponent != nullptr)
		{
			m_hoveredComponent->SetHovered(true);
		}
	}
}

bool TransformGizmo3D::BeginDrag(GizmoContext const& context, RaycastInfo const& ray)
{
	GizmoRaycastResult const result = Raycast(context, ray);
	if (!result.m_didImpact || result.m_component == nullptr)
	{
		return false;
	}

	if (m_hoveredComponent != nullptr && m_hoveredComponent != result.m_component)
	{
		m_hoveredComponent->SetHovered(false);
	}

	m_hoveredComponent = result.m_component;
	m_hoveredComponent->SetHovered(true);
	m_activeComponent = result.m_component;
	m_activeComponent->OnBeginDrag(context, result);
	return true;
}

void TransformGizmo3D::OnDrag(GizmoContext const& context, RaycastInfo const& ray)
{
	if (m_activeComponent != nullptr)
	{
		m_activeComponent->OnDrag(context, ray);
	}
}

void TransformGizmo3D::EndDrag(GizmoContext const& context)
{
	if (m_activeComponent != nullptr)
	{
		m_activeComponent->OnEndDrag(context);
		m_activeComponent = nullptr;
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
				component->RebuildVertices(context);
			}
		}
		break;
	}
	}
}

bool TransformGizmo3D::IsDragging() const { return m_activeComponent != nullptr; }

GizmoRaycastResult TransformGizmo3D::Raycast(GizmoContext const& context, RaycastInfo const& ray) const
{
	GizmoRaycastResult bestResult;
	if (context.m_selectedNode3D == nullptr)
	{
		return bestResult;
	}

	for (GizmoComponent* component : m_components)
	{
		if (!component->GetVisible())
		{
			continue;
		}

		GizmoRaycastResult const result = component->Raycast(context, ray);
		if (!result.m_didImpact)
		{
			continue;
		}

		if (!bestResult.m_didImpact || result.m_impactDist < bestResult.m_impactDist)
		{
			bestResult = result;
		}
	}

	return bestResult;
}

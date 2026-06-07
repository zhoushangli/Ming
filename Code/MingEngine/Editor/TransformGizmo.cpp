#include "MingEngine/Editor/TransformGizmo.hpp"

namespace
{
Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);
}

TransformGizmo::TransformGizmo()
{
	m_components.push_back(new GizmoAxisArrow(GizmoAxis::X, kAxisXColor));
	m_components.push_back(new GizmoAxisArrow(GizmoAxis::Y, kAxisYColor));
	m_components.push_back(new GizmoAxisArrow(GizmoAxis::Z, kAxisZColor));
	m_components.push_back(new GizmoPlaneSquare(GizmoAxis::X, kAxisXColor));
	m_components.push_back(new GizmoPlaneSquare(GizmoAxis::Y, kAxisYColor));
	m_components.push_back(new GizmoPlaneSquare(GizmoAxis::Z, kAxisZColor));
	m_components.push_back(new GizmoRotationArc(GizmoAxis::X, kAxisXColor));
	m_components.push_back(new GizmoRotationArc(GizmoAxis::Y, kAxisYColor));
	m_components.push_back(new GizmoRotationArc(GizmoAxis::Z, kAxisZColor));
}

TransformGizmo::~TransformGizmo()
{
	for (GizmoComponent* component : m_components)
	{
		delete component;
	}
	m_components.clear();
}

void TransformGizmo::UpdateHover(GizmoContext const& context, RaycastInfo const& ray)
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

bool TransformGizmo::TryBeginDrag(GizmoContext const& context, RaycastInfo const& ray)
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

void TransformGizmo::UpdateDrag(GizmoContext const& context, RaycastInfo const& ray)
{
	if (m_activeComponent != nullptr)
	{
		m_activeComponent->OnDrag(context, ray);
	}
}

void TransformGizmo::EndDrag(GizmoContext const& context)
{
	if (m_activeComponent != nullptr)
	{
		m_activeComponent->OnEndDrag(context);
		m_activeComponent = nullptr;
	}
}

void TransformGizmo::Render(GizmoContext const& context) const
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	GizmoContext renderContext = context;
	renderContext.m_isRotationActive = m_activeComponent != nullptr && m_activeComponent->IsRotationGizmo();
	renderContext.m_activeComponent  = m_activeComponent;

	for (GizmoComponent const* component : m_components)
	{
		component->Render(renderContext);
	}
}

bool TransformGizmo::IsDragging() const
{
	return m_activeComponent != nullptr;
}

GizmoRaycastResult TransformGizmo::Raycast(GizmoContext const& context, RaycastInfo const& ray)
{
	GizmoRaycastResult bestResult;
	for (GizmoComponent* component : m_components)
	{
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

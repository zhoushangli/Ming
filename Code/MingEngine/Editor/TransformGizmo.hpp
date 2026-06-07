#pragma once

#include "MingEngine/Editor/GizmoComponent.hpp"

#include <vector>

class TransformGizmo
{
public:
	TransformGizmo();
	~TransformGizmo();

	TransformGizmo(TransformGizmo const&)            = delete;
	TransformGizmo& operator=(TransformGizmo const&) = delete;

	void UpdateHover(GizmoContext const& context, RaycastInfo const& ray);
	bool TryBeginDrag(GizmoContext const& context, RaycastInfo const& ray);
	void UpdateDrag(GizmoContext const& context, RaycastInfo const& ray);
	void EndDrag(GizmoContext const& context);
	void Render(GizmoContext const& context) const;

	bool IsDragging() const;

private:
	GizmoRaycastResult Raycast(GizmoContext const& context, RaycastInfo const& ray);

private:
	std::vector<GizmoComponent*> m_components;
	GizmoComponent*              m_hoveredComponent = nullptr;
	GizmoComponent*              m_activeComponent  = nullptr;
};

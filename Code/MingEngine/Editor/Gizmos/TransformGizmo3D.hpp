#pragma once

#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"
#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"

#include <vector>

class TransformGizmo3D : public EditorGizmoVisual3D
{
	MCLASS(TransformGizmo3D, EditorGizmoVisual3D);

public:
	TransformGizmo3D();
	~TransformGizmo3D() override;

	void UpdateHover(GizmoContext const& context, RaycastInfo const& ray);
	bool TryBeginDrag(GizmoContext const& context, RaycastInfo const& ray);
	void UpdateDrag(GizmoContext const& context, RaycastInfo const& ray);
	void EndDrag(GizmoContext const& context);
	void Render(GizmoContext const& context);

	bool IsDragging() const;

	GizmoRaycastResult Raycast(GizmoContext const& context, RaycastInfo const& ray) const;

private:
	std::vector<GizmoComponent*> m_components;
	GizmoComponent* m_hoveredComponent = nullptr;
	GizmoComponent* m_activeComponent  = nullptr;
};

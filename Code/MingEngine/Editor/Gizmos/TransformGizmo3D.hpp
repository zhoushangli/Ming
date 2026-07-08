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

	void UpdateHover(GizmoContext const& context);
	bool BeginDrag(GizmoContext const& context);
	void OnDrag(GizmoContext const& context);
	void EndDrag(GizmoContext const& context);

	bool IsDragging() const;

protected:
	void OnNotification(int notification);

private:
	GizmoComponent* HitTest(GizmoContext const& context) const;

private:
	std::vector<GizmoComponent*> m_components;
	GizmoComponent*              m_hoveredComponent = nullptr;
	GizmoComponent*              m_activeComponent  = nullptr;
};

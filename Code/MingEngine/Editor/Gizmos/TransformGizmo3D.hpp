#pragma once

#include "MingEngine/Core/Math/RaycastUtils.hpp"
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
	bool BeginDragHovered(GizmoContext const& context);
	void OnDrag(GizmoContext const& context);
	void EndDrag(GizmoContext const& context);

	bool IsHovered() const;
	bool IsDragging() const;

protected:
	void OnNotification(int notification);

private:
	GizmoComponent* HitTest(GizmoContext const& context, Vector3& outHitPos) const;

private:
	std::vector<GizmoComponent*> m_components;
	GizmoComponent*              m_hoveredComponent = nullptr;
	GizmoComponent*              m_activeComponent  = nullptr;
	Vector3                      m_hoveredHitPos    = Vector3::Zero;
	MathRaycastQuery3D           m_dragStartRaycastInfo;
	NodeHandle                   m_draggedNodeHandle = NodeHandle::Invalid;
	Vector3                      m_dragStartPosition = Vector3::Zero;
	EulerAngles                  m_dragStartOrientation;
	Vector3                      m_dragStartScale = Vector3::One;
};

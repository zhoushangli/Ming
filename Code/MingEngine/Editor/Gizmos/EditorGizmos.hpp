#pragma once

#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Engine/Math/Vec2.hpp"

class Camera3D;
class TransformGizmo3D;
class ViewportAxisIndicator;
struct GizmoContext;

class EditorGizmos : public Node
{
	MCLASS(EditorGizmos, Node);

public:
	EditorGizmos();
	~EditorGizmos() override;

	void SetWorldGridVisible(bool visible);
	void SetWorldAxisVisible(bool visible);

	// --- Gizmo event routing ---
	void OnMouseMove(Camera3D const& camera, Vec2 screenPos);
	bool OnBeginDrag(Camera3D const& camera, Vec2 screenPos);
	void OnDrag(Camera3D const& camera, Vec2 screenPos);
	void OnEndDrag();
	bool IsDragging() const;
	NodeHandle Raycast(Camera3D const& camera, Vec2 screenPos) const;

protected:
	void OnReady() override;
	void OnProcess(float deltaSeconds) override;

private:
	GizmoContext BuildGizmoContext(Camera3D const& camera, Vec2 screenPos) const;

private:
	NodeHandle m_worldGridHandle           = NodeHandle::Invalid;
	NodeHandle m_worldAxisHandle           = NodeHandle::Invalid;
	TransformGizmo3D* m_transformGizmo     = nullptr;
	ViewportAxisIndicator* m_axisIndicator = nullptr;
};

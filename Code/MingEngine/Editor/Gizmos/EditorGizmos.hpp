#pragma once

#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Engine/Math/Vec2.hpp"

class Camera3D;
class TransformGizmo3D;
class ViewportAxisIndicator;

class EditorGizmos : public Node
{
	MCLASS(EditorGizmos, Node);

public:
	EditorGizmos();
	~EditorGizmos() override;

	// --- Gizmo event routing ---
	void OnMouseMove(Camera3D const& camera, Vec2 screenPos);
	bool OnBeginDrag(Camera3D const& camera, Vec2 screenPos);
	void OnDrag(Camera3D const& camera, Vec2 screenPos);
	void OnEndDrag();
	bool IsDragging() const;
	NodeHandle Raycast(Camera3D const& camera, Vec2 screenPos) const;

private:
	EditorWorldGrid3D* m_worldGrid         = nullptr;
	EditorWorldAxis3D* m_worldAxis         = nullptr;
	TransformGizmo3D* m_transformGizmo     = nullptr;
	ViewportAxisIndicator* m_axisIndicator = nullptr;
};

#pragma once

#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Core/Math/Vec2.hpp"

#include <array>

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
	bool BeginDragHovered(Camera3D const& camera, Vec2 screenPos);
	void OnDrag(Camera3D const& camera, Vec2 screenPos);
	void OnEndDrag();
	bool IsHovered() const;
	bool IsDragging() const;

private:
	EditorWorldGrid3D* m_worldGrid         = nullptr;
	std::array<EditorWorldAxis3D*, 6> m_worldAxises = {};
	TransformGizmo3D* m_transformGizmo     = nullptr;
	ViewportAxisIndicator* m_axisIndicator = nullptr;
};


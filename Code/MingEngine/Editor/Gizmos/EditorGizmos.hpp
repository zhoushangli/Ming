#pragma once

#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"
#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Core/Math/Vector2.hpp"

#include <array>
#include <map>

class Camera3D;
class TransformGizmo3D;
class ViewportAxisIndicator;
class EditorNodeGizmo3D;

class EditorGizmos : public Node
{
	MCLASS(EditorGizmos, Node);

public:
	EditorGizmos();
	~EditorGizmos() override;

	// --- Gizmo event routing ---
	void      OnMouseMove(Camera3D const& camera, Vector2 screenPos);
	bool      BeginDragHovered(Camera3D const& camera, Vector2 screenPos);
	void      OnDrag(Camera3D const& camera, Vector2 screenPos);
	void      OnEndDrag();
	bool      IsHovered() const;
	bool      IsDragging() const;
	void      CancelDrag();
	void      ClearHover();
	void      SetTool(GizmoTool tool);
	GizmoTool GetTool() const { return m_tool; }
	void      SetLocal(bool local);
	bool      IsLocal() const { return m_local; }
	void      SetGridVisible(bool visible);
	bool      IsGridVisible() const;
	ObjectID  PickNode(Camera3D const& camera, Vector2 screenPos, float maxDistance);

protected:
	void OnNotification(int notification);

private:
	void SyncNodeGizmos(GizmoContext const& context);

private:
	EditorWorldGrid3D*                     m_worldGrid      = nullptr;
	std::array<EditorWorldAxis3D*, 6>      m_worldAxises    = {};
	TransformGizmo3D*                      m_transformGizmo = nullptr;
	ViewportAxisIndicator*                 m_axisIndicator  = nullptr;
	GizmoTool                              m_tool           = GizmoTool::Transform;
	bool                                   m_local          = true;
	std::map<ObjectID, EditorNodeGizmo3D*> m_nodeGizmos;
	EditorNodeGizmo3D*                     m_activeNodeGizmo = nullptr;
	bool                                   m_nodeHovered     = false;
};

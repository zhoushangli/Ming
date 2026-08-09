#pragma once

#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"

class Camera3D;
class GizmoComponent;
class IndexBuffer;
class Node3D;
class SceneTree;
struct MathRaycastQuery3D;

enum class GizmoAxis
{
	X,
	Y,
	Z,
};

struct GizmoContext
{
	SceneTree*      m_sceneTree        = nullptr;
	Camera3D const* m_camera           = nullptr;
	NodeHandle      m_selectedNode     = NodeHandle::Invalid;
	Node3D*         m_selectedNode3D   = nullptr;
	Vector3         m_originWorld      = Vector3::Zero;
	float           m_scale            = 1.f;
	Vector2         m_clientPos        = Vector2::Zero;
	Vector2         m_clientDimensions = Vector2::Zero;
};

GizmoContext BuildGizmoContext(SceneTree* sceneTree, Camera3D const& camera, Vector2 clientPos = Vector2::Zero);

class GizmoComponent : public EditorGizmoVisual3D
{
	MCLASS(GizmoComponent, EditorGizmoVisual3D)

public:
	GizmoComponent(GizmoAxis axis, Color const& color);
	virtual ~GizmoComponent() = default;

	virtual MathRaycastResult3D Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const;
	virtual void                OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos);
	virtual void                OnDrag(
		GizmoContext const&       context,
		MathRaycastQuery3D const& startRaycastInfo,
		MathRaycastQuery3D const& currentRaycastInfo);
	virtual void OnEndDrag(GizmoContext const& context);

	Vector3 GetWorldVirtualCenter() const;

	void SetHovered(bool isHovered);
	void SetRenderPriority(int renderPriority) { m_renderPriority = renderPriority; }

	bool         IsHovered() const;
	bool         IsDragging() const;
	virtual bool IsRotationGizmo() const;

protected:
	RenderRequest SubmitRenderRequest() const override;
	void          OnNotification(int notification);
	Vector3       GetAxisWorld() const;
	Vector3       GetPlaneU() const;
	Vector3       GetPlaneV() const;
	Color         GetDrawColor() const;

protected:
	GizmoAxis m_axis       = GizmoAxis::X;
	Color     m_baseColor  = Color::White;
	bool      m_isHovered  = false;
	bool      m_isDragging = false;

	// Used for arranging the gizmos in front of each other when they overlap.
	// This is a local position
	Vector3 m_virtualCenter  = Vector3::Zero;
	int     m_renderPriority = 0;
};

class GizmoAxisArrow : public GizmoComponent
{
	MCLASS(GizmoAxisArrow, GizmoComponent)

public:
	GizmoAxisArrow(GizmoAxis axis, Color const& color);

	MathRaycastResult3D Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const override;
	void                OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos) override;
	void                OnDrag(
		GizmoContext const&       context,
		MathRaycastQuery3D const& startRaycastInfo,
		MathRaycastQuery3D const& currentRaycastInfo) override;

private:
	Vector3 m_startPosition = Vector3::Zero;
	Vector3 m_dragOrigin    = Vector3::Zero;
};

class GizmoPlaneSquare : public GizmoComponent
{
	MCLASS(GizmoPlaneSquare, GizmoComponent)

public:
	GizmoPlaneSquare(GizmoAxis axis, Color const& color);

	MathRaycastResult3D Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const override;
	void                OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos) override;
	void                OnDrag(
		GizmoContext const&       context,
		MathRaycastQuery3D const& startRaycastInfo,
		MathRaycastQuery3D const& currentRaycastInfo) override;

private:
	Vector3 m_startPosition = Vector3::Zero;
	Vector3 m_dragOrigin    = Vector3::Zero;
};

class GizmoRotationArc : public GizmoComponent
{
	MCLASS(GizmoRotationArc, GizmoComponent)

public:
	GizmoRotationArc(GizmoAxis axis, Color const& color);
	~GizmoRotationArc() override;

	MathRaycastResult3D Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const override;
	void                OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos) override;
	void                OnDrag(
		GizmoContext const&       context,
		MathRaycastQuery3D const& startRaycastInfo,
		MathRaycastQuery3D const& currentRaycastInfo) override;
	void OnEndDrag(GizmoContext const& context) override;
	bool IsRotationGizmo() const override;

protected:
	RenderRequest SubmitRenderRequest() const override;
	void          OnNotification(int notification);

private:
	std::vector<unsigned int> m_indices;
	IndexBuffer*              m_indexBuffer      = nullptr;
	EulerAngles               m_startOrientation = EulerAngles::Zero;
	Vector3                   m_dragOrigin       = Vector3::Zero;
	Vector3                   m_startVectorWorld = Vector3::Zero;
	float                     m_currentDegrees   = 0.f;
};

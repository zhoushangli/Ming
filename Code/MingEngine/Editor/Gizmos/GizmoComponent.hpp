#pragma once

#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"

class Camera3D;
class GizmoComponent;
class IndexBuffer;
class Node3D;
class SceneTree;
struct RaycastInfo;

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
	Vec3            m_originWorld      = Vec3::Zero;
	float           m_scale            = 1.f;
	Vec2            m_clientPos        = Vec2::Zero;
	Vec2            m_clientDimensions = Vec2::Zero;
};

GizmoContext BuildGizmoContext(SceneTree* sceneTree, Camera3D const& camera, Vec2 clientPos = Vec2::Zero);

class GizmoComponent : public EditorGizmoVisual3D
{
	MCLASS(GizmoComponent, EditorGizmoVisual3D)

public:
	GizmoComponent(GizmoAxis axis, Rgba8 const& color);
	virtual ~GizmoComponent() = default;

	virtual MathRaycastResult3D Raycast(GizmoContext const& context, RaycastInfo const& raycastInfo) const;
	virtual void OnBeginDrag(GizmoContext const& context, Vec3 const& hitPos);
	virtual void OnDrag(
		GizmoContext const& context, RaycastInfo const& startRaycastInfo, RaycastInfo const& currentRaycastInfo);
	virtual void OnEndDrag(GizmoContext const& context);

	Vec3 GetWorldVirtualCenter() const;

	void SetHovered(bool isHovered);
	void SetRenderPriority(int renderPriority) { m_renderPriority = renderPriority; }

	bool         IsHovered() const;
	bool         IsDragging() const;
	virtual bool IsRotationGizmo() const;

protected:
	RenderRequest SubmitRenderRequest() const override;
	void          OnNotification(int notification);
	Vec3          GetAxisWorld() const;
	Vec3          GetPlaneU() const;
	Vec3          GetPlaneV() const;
	Rgba8         GetDrawColor() const;

protected:
	GizmoAxis m_axis       = GizmoAxis::X;
	Rgba8     m_baseColor  = Rgba8::White;
	bool      m_isHovered  = false;
	bool      m_isDragging = false;

	// Used for arranging the gizmos in front of each other when they overlap.
	// This is a local position
	Vec3 m_virtualCenter  = Vec3::Zero;
	int  m_renderPriority = 0;
};

class GizmoAxisArrow : public GizmoComponent
{
	MCLASS(GizmoAxisArrow, GizmoComponent)

public:
	GizmoAxisArrow(GizmoAxis axis, Rgba8 const& color);

	MathRaycastResult3D Raycast(GizmoContext const& context, RaycastInfo const& raycastInfo) const override;
	void OnBeginDrag(GizmoContext const& context, Vec3 const& hitPos) override;
	void OnDrag(
		GizmoContext const& context, RaycastInfo const& startRaycastInfo, RaycastInfo const& currentRaycastInfo) override;

private:
	Vec3 m_startPosition = Vec3::Zero;
	Vec3 m_dragOrigin    = Vec3::Zero;
};

class GizmoPlaneSquare : public GizmoComponent
{
	MCLASS(GizmoPlaneSquare, GizmoComponent)

public:
	GizmoPlaneSquare(GizmoAxis axis, Rgba8 const& color);

	MathRaycastResult3D Raycast(GizmoContext const& context, RaycastInfo const& raycastInfo) const override;
	void OnBeginDrag(GizmoContext const& context, Vec3 const& hitPos) override;
	void OnDrag(
		GizmoContext const& context, RaycastInfo const& startRaycastInfo, RaycastInfo const& currentRaycastInfo) override;

private:
	Vec3 m_startPosition = Vec3::Zero;
	Vec3 m_dragOrigin    = Vec3::Zero;
};

class GizmoRotationArc : public GizmoComponent
{
	MCLASS(GizmoRotationArc, GizmoComponent)

public:
	GizmoRotationArc(GizmoAxis axis, Rgba8 const& color);
	~GizmoRotationArc() override;

	MathRaycastResult3D Raycast(GizmoContext const& context, RaycastInfo const& raycastInfo) const override;
	void OnBeginDrag(GizmoContext const& context, Vec3 const& hitPos) override;
	void OnDrag(
		GizmoContext const& context, RaycastInfo const& startRaycastInfo, RaycastInfo const& currentRaycastInfo) override;
	void OnEndDrag(GizmoContext const& context) override;
	bool IsRotationGizmo() const override;

protected:
	RenderRequest SubmitRenderRequest() const override;
	void OnNotification(int notification);

private:
	std::vector<unsigned int> m_indices;
	IndexBuffer*              m_indexBuffer      = nullptr;
	EulerAngles               m_startOrientation = EulerAngles::Zero;
	Vec3                      m_dragOrigin        = Vec3::Zero;
	Vec3                      m_startVectorWorld  = Vec3::Zero;
	float                     m_currentDegrees    = 0.f;
};

#pragma once

#include "MingEngine/Scene/Core/NodeHandle.hpp"
#include "MingEngine/Scene/Physics/NodeRaycastUtils.hpp"

#include "MingEngine/Engine/Render/Rgba8.hpp"
#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/Vec2.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"

class Camera3D;
class GizmoComponent;
class Node3D;
class SceneTree;

enum class GizmoAxis
{
	X,
	Y,
	Z,
};

struct GizmoContext
{
	SceneTree*            m_sceneTree        = nullptr;
	Camera3D const*       m_camera           = nullptr;
	NodeHandle            m_selectedNode     = NodeHandle::Invalid;
	Node3D*               m_selectedNode3D   = nullptr;
	Vec3                  m_originWorld      = Vec3::Zero;
	float                 m_scale            = 1.f;
	Vec2                  m_clientPos        = Vec2::Zero;
	Vec2                  m_clientDimensions = Vec2::Zero;
	bool                  m_isRotationActive = false;
	GizmoComponent const* m_activeComponent  = nullptr;
};

struct GizmoRaycastResult
{
	bool            m_didImpact  = false;
	float           m_impactDist = 0.f;
	Vec3            m_impactPos  = Vec3::Zero;
	GizmoComponent* m_component  = nullptr;
};

class GizmoComponent
{
public:
	GizmoComponent(GizmoAxis axis, Rgba8 const& color);
	virtual ~GizmoComponent() = default;

	virtual GizmoRaycastResult Raycast(GizmoContext const& context, RaycastInfo const& ray) = 0;
	virtual void               Render(GizmoContext const& context) const                    = 0;
	virtual void               OnBeginDrag(GizmoContext const& context, GizmoRaycastResult const& hit);
	virtual void               OnDrag(GizmoContext const& context, RaycastInfo const& ray);
	virtual void               OnEndDrag(GizmoContext const& context);

	void         SetHovered(bool isHovered);
	bool         IsHovered() const;
	bool         IsDragging() const;
	virtual bool IsRotationGizmo() const;

protected:
	Vec3  GetAxisWorld() const;
	Vec3  GetPlaneU() const;
	Vec3  GetPlaneV() const;
	Rgba8 GetDrawColor() const;

protected:
	GizmoAxis m_axis       = GizmoAxis::X;
	Rgba8     m_baseColor  = Rgba8::White;
	bool      m_isHovered  = false;
	bool      m_isDragging = false;
};

class GizmoAxisArrow : public GizmoComponent
{
public:
	GizmoAxisArrow(GizmoAxis axis, Rgba8 const& color);

	GizmoRaycastResult Raycast(GizmoContext const& context, RaycastInfo const& ray) override;
	void               Render(GizmoContext const& context) const override;
	void               OnBeginDrag(GizmoContext const& context, GizmoRaycastResult const& hit) override;
	void               OnDrag(GizmoContext const& context, RaycastInfo const& ray) override;

private:
	Vec3  m_startPosition = Vec3::Zero;
	Vec3  m_dragOrigin    = Vec3::Zero;
	float m_startAxisT    = 0.f;
};

class GizmoPlaneSquare : public GizmoComponent
{
public:
	GizmoPlaneSquare(GizmoAxis axis, Rgba8 const& color);

	GizmoRaycastResult Raycast(GizmoContext const& context, RaycastInfo const& ray) override;
	void               Render(GizmoContext const& context) const override;
	void               OnBeginDrag(GizmoContext const& context, GizmoRaycastResult const& hit) override;
	void               OnDrag(GizmoContext const& context, RaycastInfo const& ray) override;

private:
	Vec3 m_startPosition = Vec3::Zero;
	Vec3 m_startHitWorld = Vec3::Zero;
};

class GizmoRotationArc : public GizmoComponent
{
public:
	GizmoRotationArc(GizmoAxis axis, Rgba8 const& color);

	GizmoRaycastResult Raycast(GizmoContext const& context, RaycastInfo const& ray) override;
	void               Render(GizmoContext const& context) const override;
	void               OnBeginDrag(GizmoContext const& context, GizmoRaycastResult const& hit) override;
	void               OnDrag(GizmoContext const& context, RaycastInfo const& ray) override;
	void               OnEndDrag(GizmoContext const& context) override;
	bool               IsRotationGizmo() const override;

private:
	EulerAngles m_startOrientation = EulerAngles::Zero;
	Vec3        m_dragOrigin       = Vec3::Zero;
	Vec3        m_startVectorWorld = Vec3::Zero;
	Vec3        m_currentHitWorld  = Vec3::Zero;
	float       m_currentDegrees   = 0.f;
};

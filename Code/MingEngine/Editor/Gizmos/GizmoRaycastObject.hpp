#pragma once

#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/Quad3.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

class GizmoComponent;

class GizmoRaycastObject : public RaycastObject
{
public:
	GizmoRaycastObject(GizmoComponent* owner);
	virtual ~GizmoRaycastObject() = default;

	RaycastResult3D IntersectBounds(RaycastQuery3D const& query) override;

	GizmoComponent* GetOwner() const { return m_component; }

protected:
	GizmoComponent* m_component = nullptr;
	AABB3           m_worldAABB;
};

class GizmoAxisRaycastObject : public GizmoRaycastObject
{
public:
	GizmoAxisRaycastObject(GizmoComponent* owner);

	RaycastResult3D IntersectRay(RaycastQuery3D const& query) override;

private:
	friend class GizmoAxisArrow;

	Vec3  m_arrowStart = Vec3::Zero;
	Vec3  m_arrowEnd   = Vec3::Zero;
	float m_arrowRad   = 0.f;
};

class GizmoPlaneRaycastObject : public GizmoRaycastObject
{
public:
	GizmoPlaneRaycastObject(GizmoComponent* owner);

	RaycastResult3D IntersectRay(RaycastQuery3D const& query) override;

private:
	friend class GizmoPlaneSquare;

	Quad3 m_quad;
};

class GizmoArcRaycastObject : public GizmoRaycastObject
{
public:
	GizmoArcRaycastObject(GizmoComponent* owner);

	RaycastResult3D IntersectRay(RaycastQuery3D const& query) override;

private:
	friend class GizmoRotationArc;

	Vec3  m_worldOrigin = Vec3::Zero;
	Vec3  m_worldU      = Vec3::Zero;
	Vec3  m_worldV      = Vec3::Zero;
	float m_radius      = 0.f;
	float m_tubeRadius  = 0.f;
	Vec3  m_worldNormal = Vec3::Zero;
};

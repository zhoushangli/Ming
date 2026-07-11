#include "MingEngine/Editor/Gizmos/GizmoRaycastObject.hpp"

#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"

using namespace Math;

GizmoRaycastObject::GizmoRaycastObject(GizmoComponent* owner) : m_component(owner) {}

RaycastResult3D GizmoRaycastObject::IntersectBounds(RaycastQuery3D const& query)
{
	RaycastResult3D hit;
	hit.m_owner = m_owner;

	(MathRaycastResult3D&)hit = RaycastVsAABB3D(query.m_start, query.m_direction, query.m_maxDistance, m_worldAABB);

	return hit;
}

GizmoAxisRaycastObject::GizmoAxisRaycastObject(GizmoComponent* owner) : GizmoRaycastObject(owner) {}

RaycastResult3D GizmoAxisRaycastObject::IntersectRay(RaycastQuery3D const& query)
{
	RaycastResult3D hit;
	hit.m_owner = m_owner;

	(MathRaycastResult3D&)hit = RaycastVsCylinder3D(
		query.m_start,
		query.m_direction,
		query.m_maxDistance,
		m_arrowStart,
		m_arrowEnd,
		m_arrowRad);
	hit.m_owner = m_owner;
	return hit;
}

GizmoPlaneRaycastObject::GizmoPlaneRaycastObject(GizmoComponent* owner) : GizmoRaycastObject(owner) {}

RaycastResult3D GizmoPlaneRaycastObject::IntersectRay(RaycastQuery3D const& query)
{
	RaycastResult3D hit;
	hit.m_owner               = m_owner;
	(MathRaycastResult3D&)hit = RaycastVsQuad3D(query.m_start, query.m_direction, query.m_maxDistance, m_quad);
	hit.m_owner               = m_owner;
	return hit;
}

GizmoArcRaycastObject::GizmoArcRaycastObject(GizmoComponent* owner) : GizmoRaycastObject(owner) {}

RaycastResult3D GizmoArcRaycastObject::IntersectRay(RaycastQuery3D const& query)
{
	RaycastResult3D result;
	result.m_owner = m_owner;

	MathRaycastResult3D const planeHit =
		RaycastVsPlane3D(query.m_start, query.m_direction, query.m_maxDistance, m_worldOrigin, m_worldNormal);
	if (!planeHit.m_didImpact)
	{
		return result;
	}

	Vec3 const  fromOrigin = planeHit.m_impactPos - m_worldOrigin;
	float const hitRadius  = fromOrigin.GetLength();
	if (Abs(hitRadius - m_radius) > m_tubeRadius)
	{
		return result;
	}

	result.m_didImpact    = true;
	result.m_impactDist   = planeHit.m_impactDist;
	result.m_impactPos    = planeHit.m_impactPos;
	result.m_impactNormal = m_worldNormal;
	if (DotProduct3D(result.m_impactNormal, query.m_direction) > 0.f)
	{
		result.m_impactNormal = -result.m_impactNormal;
	}
	return result;
}

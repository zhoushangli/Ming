#include "MingEngine/Editor/Gizmos/GizmoRaycastObject.hpp"

#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"

using namespace Math;

namespace
{
float constexpr kGizmoAxisLength        = 1.25f;
float constexpr kGizmoArrowRadius       = 0.035f;
float constexpr kGizmoAxisPickRadius    = 0.11f;
float constexpr kGizmoPlaneOffset       = 0.36f;
float constexpr kGizmoPlaneSize         = 0.22f;
float constexpr kGizmoPlanePickSize     = 0.30f;
float constexpr kGizmoRotationRadius    = 0.88f;
float constexpr kGizmoRotationPickWidth = 0.10f;

// --- Raycast math helpers ---

float GetRayPointAxisT(Vec3 const& rayStart, Vec3 const& rayForward, Vec3 const& lineOrigin, Vec3 const& lineAxis)
{
	Vec3 const  w0          = rayStart - lineOrigin;
	float const a           = DotProduct3D(rayForward, rayForward);
	float const b           = DotProduct3D(rayForward, lineAxis);
	float const c           = DotProduct3D(lineAxis, lineAxis);
	float const d           = DotProduct3D(rayForward, w0);
	float const e           = DotProduct3D(lineAxis, w0);
	float const denominator = a * c - b * b;
	if (Abs(denominator) <= 0.000001f)
	{
		return e / c;
	}

	return (a * e - b * d) / denominator;
}

float GetDistanceRayToSegment(
	Vec3 const& rayStart,
	Vec3 const& rayForward,
	Vec3 const& segmentStart,
	Vec3 const& segmentEnd,
	float&      outRayT,
	float&      outSegmentT)
{
	Vec3 const  segment    = segmentEnd - segmentStart;
	float const segmentLen = segment.GetLength();
	if (segmentLen <= 0.000001f)
	{
		outRayT     = 0.f;
		outSegmentT = 0.f;
		return GetDistance3D(rayStart, segmentStart);
	}

	Vec3 const segmentAxis = segment / segmentLen;
	outSegmentT            = GetRayPointAxisT(rayStart, rayForward, segmentStart, segmentAxis);
	outSegmentT            = GetClamped(outSegmentT, 0.f, segmentLen);
	Vec3 const segmentPos  = segmentStart + segmentAxis * outSegmentT;
	outRayT                = DotProduct3D(segmentPos - rayStart, rayForward);
	outRayT                = Max(0.f, outRayT);
	Vec3 const rayPos      = rayStart + rayForward * outRayT;
	return GetDistance3D(rayPos, segmentPos);
}

bool RaycastPlaneRaw(
	Vec3 const& rayStart,
	Vec3 const& rayForward,
	float       maxLength,
	Vec3 const& planePoint,
	Vec3 const& planeNormal,
	Vec3&       outHit,
	float&      outDist)
{
	float const denominator = DotProduct3D(rayForward, planeNormal);
	if (Abs(denominator) <= 0.000001f)
	{
		return false;
	}

	outDist = DotProduct3D(planePoint - rayStart, planeNormal) / denominator;
	if (outDist < 0.f || outDist > maxLength)
	{
		return false;
	}

	outHit = rayStart + rayForward * outDist;
	return true;
}

Vec3 GetGizmoAxisWorld(GizmoAxis axis)
{
	switch (axis)
	{
	case GizmoAxis::X:
		return Vec3::Forward;
	case GizmoAxis::Y:
		return Vec3::Left;
	case GizmoAxis::Z:
		return Vec3::Up;
	}

	return Vec3::Forward;
}

Vec3 GetGizmoPlaneU(GizmoAxis axis)
{
	switch (axis)
	{
	case GizmoAxis::X:
		return Vec3::Left;
	case GizmoAxis::Y:
		return Vec3::Forward;
	case GizmoAxis::Z:
		return Vec3::Forward;
	}

	return Vec3::Forward;
}

Vec3 GetGizmoPlaneV(GizmoAxis axis)
{
	switch (axis)
	{
	case GizmoAxis::X:
		return Vec3::Up;
	case GizmoAxis::Y:
		return Vec3::Up;
	case GizmoAxis::Z:
		return Vec3::Left;
	}

	return Vec3::Up;
}

} // namespace

// ============================================================
// GizmoCylinderShape
// ============================================================

GizmoCylinderShape::GizmoCylinderShape(Vec3 const& worldStart, Vec3 const& worldEnd, float radius)
	: m_worldStart(worldStart), m_worldEnd(worldEnd), m_radius(radius)
{
}

AABB3 GizmoCylinderShape::GetWorldAABB() const
{
	Vec3 const mins = Vec3(
		Min(m_worldStart.x, m_worldEnd.x) - m_radius,
		Min(m_worldStart.y, m_worldEnd.y) - m_radius,
		Min(m_worldStart.z, m_worldEnd.z) - m_radius);
	Vec3 const maxs = Vec3(
		Max(m_worldStart.x, m_worldEnd.x) + m_radius,
		Max(m_worldStart.y, m_worldEnd.y) + m_radius,
		Max(m_worldStart.z, m_worldEnd.z) + m_radius);
	return AABB3(mins, maxs);
}

bool GizmoCylinderShape::IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const
{
	float       rayT  = 0.f;
	float       axisT = 0.f;
	float const dist = GetDistanceRayToSegment(query.m_start, query.m_direction, m_worldStart, m_worldEnd, rayT, axisT);

	if (dist > m_radius || rayT > query.m_maxDistance)
	{
		return false;
	}

	outDist = rayT;
	outPos  = query.m_start + query.m_direction * rayT;
	return true;
}

// ============================================================
// GizmoConeShape
// ============================================================

GizmoConeShape::GizmoConeShape(Vec3 const& worldStart, Vec3 const& worldEnd, float radius)
	: m_worldStart(worldStart), m_worldEnd(worldEnd), m_radius(radius)
{
}

AABB3 GizmoConeShape::GetWorldAABB() const
{
	Vec3 const mins = Vec3(
		Min(m_worldStart.x, m_worldEnd.x) - m_radius,
		Min(m_worldStart.y, m_worldEnd.y) - m_radius,
		Min(m_worldStart.z, m_worldEnd.z) - m_radius);
	Vec3 const maxs = Vec3(
		Max(m_worldStart.x, m_worldEnd.x) + m_radius,
		Max(m_worldStart.y, m_worldEnd.y) + m_radius,
		Max(m_worldStart.z, m_worldEnd.z) + m_radius);
	return AABB3(mins, maxs);
}

bool GizmoConeShape::IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const
{
	float       rayT  = 0.f;
	float       axisT = 0.f;
	float const dist = GetDistanceRayToSegment(query.m_start, query.m_direction, m_worldStart, m_worldEnd, rayT, axisT);

	// Taper the pick radius from full at base to near-zero at tip
	float const length     = Max((m_worldEnd - m_worldStart).GetLength(), 0.0001f);
	float const taper      = GetClamped(axisT / length, 0.f, 1.f);
	float const pickRadius = m_radius * (1.f - taper * 0.9f);

	if (dist > pickRadius || rayT > query.m_maxDistance)
	{
		return false;
	}

	outDist = rayT;
	outPos  = query.m_start + query.m_direction * rayT;
	return true;
}

// ============================================================
// GizmoQuadShape
// ============================================================

GizmoQuadShape::GizmoQuadShape(
	Vec3 const& worldCenter, Vec3 const& worldU, Vec3 const& worldV, float halfExtent, Vec3 const& worldNormal)
	: m_worldCenter(worldCenter), m_worldU(worldU), m_worldV(worldV), m_worldNormal(worldNormal.GetNormalized()),
	  m_halfExtent(halfExtent)
{
}

AABB3 GizmoQuadShape::GetWorldAABB() const
{
	Vec3 const corner = m_worldU * m_halfExtent + m_worldV * m_halfExtent;
	Vec3 const a      = m_worldCenter + corner;
	Vec3 const b      = m_worldCenter - corner;
	return AABB3(Vec3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z)), Vec3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z)));
}

bool GizmoQuadShape::IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const
{
	Vec3  hit;
	float dist = 0.f;
	if (!RaycastPlaneRaw(
			query.m_start,
			query.m_direction,
			query.m_maxDistance,
			m_worldCenter,
			m_worldNormal,
			hit,
			dist))
	{
		return false;
	}

	Vec3 const local = hit - m_worldCenter;
	if (Abs(DotProduct3D(local, m_worldU)) > m_halfExtent || Abs(DotProduct3D(local, m_worldV)) > m_halfExtent)
	{
		return false;
	}

	outDist = dist;
	outPos  = hit;
	return true;
}

// ============================================================
// GizmoTorusArcShape
// ============================================================

GizmoTorusArcShape::GizmoTorusArcShape(
	Vec3 const& worldOrigin,
	Vec3 const& worldU,
	Vec3 const& worldV,
	float       radius,
	float       tubeRadius,
	Vec3 const& worldNormal)
	: m_worldOrigin(worldOrigin), m_worldU(worldU), m_worldV(worldV), m_radius(radius), m_tubeRadius(tubeRadius),
	  m_worldNormal(worldNormal.GetNormalized())
{
}

AABB3 GizmoTorusArcShape::GetWorldAABB() const
{
	float const outer = m_radius + m_tubeRadius;
	return AABB3(m_worldOrigin - Vec3(outer, outer, outer), m_worldOrigin + Vec3(outer, outer, outer));
}

bool GizmoTorusArcShape::IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const
{
	// Intersect with the torus plane first
	Vec3  hit;
	float dist = 0.f;
	if (!RaycastPlaneRaw(
			query.m_start,
			query.m_direction,
			query.m_maxDistance,
			m_worldOrigin,
			m_worldNormal,
			hit,
			dist))
	{
		return false;
	}

	Vec3 const  fromOrigin = hit - m_worldOrigin;
	float const hitRadius  = fromOrigin.GetLength();
	if (Abs(hitRadius - m_radius) > m_tubeRadius)
	{
		return false;
	}

	outDist = dist;
	outPos  = hit;
	return true;
}

// ============================================================
// GizmoRaycastObject
// ============================================================

GizmoRaycastObject::GizmoRaycastObject(GizmoComponent* owner) : m_component(owner) {}

GizmoRaycastObject::~GizmoRaycastObject() { ClearShapes(); }

void GizmoRaycastObject::ClearShapes()
{
	for (GizmoShape* shape : m_shapes)
	{
		delete shape;
	}
	m_shapes.clear();
	m_worldAABB = AABB3::Zero;
}

void GizmoRaycastObject::Rebuild(GizmoAxis axis, Vec3 const& originWorld, float scale)
{
	ClearShapes();

	Vec3 const axisWorld = GetGizmoAxisWorld(axis);
	Vec3 const planeU    = GetGizmoPlaneU(axis);
	Vec3 const planeV    = GetGizmoPlaneV(axis);

	// 1) Arrow shaft (cylinder) and tip (cone)
	Vec3 const  arrowStart = originWorld;
	Vec3 const  arrowEnd   = originWorld + axisWorld * (kGizmoAxisLength * scale);
	float const arrowRad   = kGizmoAxisPickRadius * scale;
	m_shapes.push_back(new GizmoCylinderShape(arrowStart, arrowEnd, arrowRad));
	m_shapes.push_back(
		new GizmoConeShape(arrowEnd, arrowEnd + axisWorld * (kGizmoArrowRadius * 4.f * scale), arrowRad * 1.5f));

	// 2) Plane square
	Vec3 const  planeCenter = originWorld + (planeU + planeV) * (kGizmoPlaneOffset * scale);
	float const planeHalf   = kGizmoPlanePickSize * scale * 0.5f;
	m_shapes.push_back(new GizmoQuadShape(planeCenter, planeU, planeV, planeHalf, axisWorld));

	// 3) Rotation arc (torus)
	float const rotRadius    = kGizmoRotationRadius * scale;
	float const rotHalfWidth = kGizmoRotationPickWidth * scale;
	m_shapes.push_back(new GizmoTorusArcShape(originWorld, planeU, planeV, rotRadius, rotHalfWidth, axisWorld));

	// 4) Compute world AABB from all shapes
	if (!m_shapes.empty())
	{
		m_worldAABB = m_shapes[0]->GetWorldAABB();
		for (size_t i = 1; i < m_shapes.size(); ++i)
		{
			AABB3 const shapeAABB = m_shapes[i]->GetWorldAABB();
			m_worldAABB.m_mins.x  = Min(m_worldAABB.m_mins.x, shapeAABB.m_mins.x);
			m_worldAABB.m_mins.y  = Min(m_worldAABB.m_mins.y, shapeAABB.m_mins.y);
			m_worldAABB.m_mins.z  = Min(m_worldAABB.m_mins.z, shapeAABB.m_mins.z);
			m_worldAABB.m_maxs.x  = Max(m_worldAABB.m_maxs.x, shapeAABB.m_maxs.x);
			m_worldAABB.m_maxs.y  = Max(m_worldAABB.m_maxs.y, shapeAABB.m_maxs.y);
			m_worldAABB.m_maxs.z  = Max(m_worldAABB.m_maxs.z, shapeAABB.m_maxs.z);
		}
	}
	else
	{
		m_worldAABB = AABB3::Zero;
	}
}

SceneRaycastResult3D GizmoRaycastObject::IntersectBounds(RaycastQuery3D const& query)
{
	SceneRaycastResult3D hit;
	hit.m_owner = m_owner;

	(RaycastResult3D&)hit = RaycastVsAABB3D(query.m_start, query.m_direction, query.m_maxDistance, m_worldAABB);

	return hit;
}

SceneRaycastResult3D GizmoRaycastObject::IntersectRay(RaycastQuery3D const& query)
{
	SceneRaycastResult3D bestHit;
	bestHit.m_owner = m_owner;

	for (GizmoShape* shape : m_shapes)
	{
		float dist = 0.f;
		Vec3  pos;
		if (shape->IntersectRay(query, dist, pos))
		{
			if (!bestHit.m_didImpact || dist < bestHit.m_impactDist)
			{
				bestHit.m_didImpact    = true;
				bestHit.m_impactDist   = dist;
				bestHit.m_impactPos    = pos;
				bestHit.m_impactNormal = Vec3::Zero;
			}
		}
	}

	return bestHit;
}

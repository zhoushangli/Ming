#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/Cone3.hpp"
#include "MingEngine/Core/Math/Disc2.hpp"
#include "MingEngine/Core/Math/FloatRange.hpp"
#include "MingEngine/Core/Math/LineSegment2.hpp"
#include "MingEngine/Core/Math/Quad3.hpp"
#include "MingEngine/Core/Math/Sphere3.hpp"
#include "MingEngine/Core/Math/Triangle3.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

struct MathRaycastQuery3D
{
	Vector3 m_startPos;
	Vector3 m_forwardNormal;
	float   m_maxLength = 1.f;
};

struct MathRaycastResult2D
{
	MathRaycastResult2D() = default;
	MathRaycastResult2D(Vector2 rayStartPos, Vector2 rayFwdNormal, float rayMaxLength)
		: m_rayStartPos(rayStartPos), m_rayFwdNormal(rayFwdNormal), m_rayMaxLength(rayMaxLength)
	{
	}

	// Basic raycast result information (required)
	bool    m_didImpact  = false;
	float   m_impactDist = 0.f;
	Vector2 m_impactPos;
	Vector2 m_impactNormal;

	// Original raycast information (optional)
	Vector2 m_rayStartPos;
	Vector2 m_rayFwdNormal;
	float   m_rayMaxLength = 1.f;
};

struct MathRaycastResult3D
{
	MathRaycastResult3D() = default;
	MathRaycastResult3D(Vector3 rayStartPos, Vector3 rayFwdNormal, float rayMaxLength)
		: m_rayStartPos(rayStartPos), m_rayFwdNormal(rayFwdNormal), m_rayMaxLength(rayMaxLength)
	{
	}

	// Basic raycast result information (required)
	bool    m_didImpact  = false;
	float   m_impactDist = 0.f;
	Vector3 m_impactPos;
	Vector3 m_impactNormal;

	// Original raycast information (optional)
	Vector3 m_rayStartPos;
	Vector3 m_rayFwdNormal;
	float   m_rayMaxLength = 1.f;
};

MathRaycastResult2D RaycastVsDisc2D(
	Vector2 startPos, Vector2 forwardNormal, float maxDist, Vector2 discCenter, float discRadius);
MathRaycastResult2D RaycastVsDisc2D(Vector2 startPos, Vector2 forwardNormal, float maxDist, Disc2 disc);
MathRaycastResult2D RaycastVsLineSegments2D(
	Vector2 startPos, Vector2 forwardNormal, float maxDist, Vector2 lineStartPos, Vector2 lineEndPos);
MathRaycastResult2D RaycastVsLineSegments2D(Vector2 startPos, Vector2 forwardNormal, float maxDist, LineSegment2 line);
MathRaycastResult2D RaycastVsAABB2D(
	Vector2 startPos, Vector2 forwardNormal, float maxDist, Vector2 aabbMins, Vector2 aabbMaxs);
MathRaycastResult2D RaycastVsAABB2D(Vector2 startPos, Vector2 forwardNormal, float maxDist, AABB2 aabb);

MathRaycastResult3D RaycastVsAABB3D(Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, AABB3 box);

MathRaycastResult3D RaycastVsSphere3D(
	Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, Vector3 sphereCenter, float sphereRadius);

MathRaycastResult3D RaycastVsSphere3D(Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, Sphere3 sphere);

MathRaycastResult3D RaycastVsCapsule3D(
	Vector3 rayStart,
	Vector3 rayForwardNormal,
	float   rayLength,
	Vector3 capsuleStart,
	Vector3 capsuleEnd,
	float   capsuleRadius);

MathRaycastResult3D RaycastVsCapsule3D(Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, Capsule3 capsule);

MathRaycastResult3D RaycastVsCylinderZ3D(
	Vector3           rayStart,
	Vector3           rayForwardNormal,
	float             rayLength,
	Vector2 const&    centerXY,
	FloatRange const& minMaxZ,
	float             radiusXY);

MathRaycastResult3D RaycastVsCylinder3D(
	Vector3        rayStart,
	Vector3        rayForwardNormal,
	float          rayLength,
	Vector3 const& cylinderStart,
	Vector3 const& cylinderEnd,
	float          radiusXY);

MathRaycastResult3D RaycastVsCone3D(
	Vector3        rayStart,
	Vector3        rayForwardNormal,
	float          rayLength,
	Vector3 const& coneStart,
	Vector3 const& coneEnd,
	float          radius);

MathRaycastResult3D RaycastVsCone3D(Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, Cone3 const& cone);

MathRaycastResult3D RaycastVsPlane3D(
	Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, Vector3 const& planePoint, Vector3 const& planeNormal);

MathRaycastResult3D RaycastVsTriangle3D(
	Vector3        rayStart,
	Vector3        rayForwardNormal,
	float          rayLength,
	Vector3 const& v0,
	Vector3 const& v1,
	Vector3 const& v2);

MathRaycastResult3D RaycastVsTriangle3D(
	Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, Triangle3 const& triangle);

MathRaycastResult3D RaycastVsQuad3D(
	Vector3        rayStart,
	Vector3        rayForwardNormal,
	float          rayLength,
	Vector3 const& p0,
	Vector3 const& p1,
	Vector3 const& p2,
	Vector3 const& p3);

MathRaycastResult3D RaycastVsQuad3D(Vector3 rayStart, Vector3 rayForwardNormal, float rayLength, Quad3 const& quad);

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
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

struct MathRaycastQuery3D
{
	Vec3  m_startPos;
	Vec3  m_forwardNormal;
	float m_maxLength = 1.f;
};

struct MathRaycastResult2D
{
	MathRaycastResult2D() = default;
	MathRaycastResult2D(Vec2 rayStartPos, Vec2 rayFwdNormal, float rayMaxLength)
		: m_rayStartPos(rayStartPos), m_rayFwdNormal(rayFwdNormal), m_rayMaxLength(rayMaxLength)
	{
	}

	// Basic raycast result information (required)
	bool  m_didImpact  = false;
	float m_impactDist = 0.f;
	Vec2  m_impactPos;
	Vec2  m_impactNormal;

	// Original raycast information (optional)
	Vec2  m_rayStartPos;
	Vec2  m_rayFwdNormal;
	float m_rayMaxLength = 1.f;
};

struct MathRaycastResult3D
{
	MathRaycastResult3D() = default;
	MathRaycastResult3D(Vec3 rayStartPos, Vec3 rayFwdNormal, float rayMaxLength)
		: m_rayStartPos(rayStartPos), m_rayFwdNormal(rayFwdNormal), m_rayMaxLength(rayMaxLength)
	{
	}

	// Basic raycast result information (required)
	bool  m_didImpact  = false;
	float m_impactDist = 0.f;
	Vec3  m_impactPos;
	Vec3  m_impactNormal;

	// Original raycast information (optional)
	Vec3  m_rayStartPos;
	Vec3  m_rayFwdNormal;
	float m_rayMaxLength = 1.f;
};

MathRaycastResult2D RaycastVsDisc2D(
	Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 discCenter, float discRadius);
MathRaycastResult2D RaycastVsDisc2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Disc2 disc);
MathRaycastResult2D RaycastVsLineSegments2D(
	Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 lineStartPos, Vec2 lineEndPos);
MathRaycastResult2D RaycastVsLineSegments2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, LineSegment2 line);
MathRaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 aabbMins, Vec2 aabbMaxs);
MathRaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, AABB2 aabb);

MathRaycastResult3D RaycastVsAABB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, AABB3 box);

MathRaycastResult3D RaycastVsSphere3D(
	Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 sphereCenter, float sphereRadius);

MathRaycastResult3D RaycastVsSphere3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Sphere3 sphere);

MathRaycastResult3D RaycastVsCapsule3D(
	Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 capsuleStart, Vec3 capsuleEnd, float capsuleRadius);

MathRaycastResult3D RaycastVsCapsule3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Capsule3 capsule);

MathRaycastResult3D RaycastVsCylinderZ3D(
	Vec3              rayStart,
	Vec3              rayForwardNormal,
	float             rayLength,
	Vec2 const&       centerXY,
	FloatRange const& minMaxZ,
	float             radiusXY);

MathRaycastResult3D RaycastVsCylinder3D(
	Vec3        rayStart,
	Vec3        rayForwardNormal,
	float       rayLength,
	Vec3 const& cylinderStart,
	Vec3 const& cylinderEnd,
	float       radiusXY);

MathRaycastResult3D RaycastVsCone3D(
	Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 const& coneStart, Vec3 const& coneEnd, float radius);

MathRaycastResult3D RaycastVsCone3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Cone3 const& cone);

MathRaycastResult3D RaycastVsPlane3D(
	Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 const& planePoint, Vec3 const& planeNormal);

MathRaycastResult3D RaycastVsTriangle3D(
	Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 const& v0, Vec3 const& v1, Vec3 const& v2);

MathRaycastResult3D RaycastVsTriangle3D(
	Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Triangle3 const& triangle);

MathRaycastResult3D RaycastVsQuad3D(
	Vec3        rayStart,
	Vec3        rayForwardNormal,
	float       rayLength,
	Vec3 const& p0,
	Vec3 const& p1,
	Vec3 const& p2,
	Vec3 const& p3);

MathRaycastResult3D RaycastVsQuad3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Quad3 const& quad);

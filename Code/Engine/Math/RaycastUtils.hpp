#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Sphere3.hpp"

struct RaycastResult2D
{
    RaycastResult2D() = default;
    RaycastResult2D(Vec2 rayStartPos, Vec2 rayFwdNormal, float rayMaxLength) 
        : m_rayStartPos(rayStartPos), m_rayFwdNormal(rayFwdNormal), m_rayMaxLength(rayMaxLength) {}

    // Basic raycast result information (required)
    bool	m_didImpact = false;
    float	m_impactDist = 0.f;
    Vec2	m_impactPos;
    Vec2	m_impactNormal;

    // Original raycast information (optional)
    Vec2	m_rayStartPos;
    Vec2	m_rayFwdNormal;
    float	m_rayMaxLength = 1.f;
};

struct RaycastResult3D
{
    RaycastResult3D() = default;
    RaycastResult3D(Vec3 rayStartPos, Vec3 rayFwdNormal, float rayMaxLength)
        : m_rayStartPos(rayStartPos), m_rayFwdNormal(rayFwdNormal), m_rayMaxLength(rayMaxLength) {}

    // Basic raycast result information (required)
    bool	m_didImpact = false;
    float	m_impactDist = 0.f;
    Vec3	m_impactPos;
    Vec3	m_impactNormal;

    // Original raycast information (optional)
    Vec3	m_rayStartPos;
    Vec3	m_rayFwdNormal;
    float	m_rayMaxLength = 1.f;
};

RaycastResult2D RaycastVsDisc2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 discCenter, float discRadius);
RaycastResult2D RaycastVsDisc2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Disc2 disc);
RaycastResult2D RaycastVsLineSegments2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 lineStartPos, Vec2 lineEndPos);
RaycastResult2D RaycastVsLineSegments2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, LineSegment2 line);
RaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 aabbMins, Vec2 aabbMaxs);
RaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, AABB2 aabb);

RaycastResult3D RaycastVsAABB3D(
    Vec3 rayStart,
    Vec3 rayForwardNormal,
    float rayLength,
    AABB3 box
);

RaycastResult3D RaycastVsSphere3D(
    Vec3 rayStart,
    Vec3 rayForwardNormal,
    float rayLength,
    Vec3 sphereCenter,
    float sphereRadius
);

RaycastResult3D RaycastVsSphere3D(
    Vec3 rayStart,
    Vec3 rayForwardNormal,
    float rayLength,
    Sphere3 sphere
);

RaycastResult3D RaycastVsCylinderZ3D(
    Vec3 rayStart,
    Vec3 rayForwardNormal,
    float rayLength,
    Vec2 const& centerXY,
    FloatRange const& minMaxZ,
    float radiusXY
);

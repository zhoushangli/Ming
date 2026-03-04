#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/LineSegment2.hpp"

struct RaycastResult2D
{
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

RaycastResult2D RaycastVsDisc2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 discCenter, float discRadius);
RaycastResult2D RaycastVsDisc2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Disc2 disc);
RaycastResult2D RaycastVsLineSegments2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 lineStartPos, Vec2 lineEndPos);
RaycastResult2D RaycastVsLineSegments2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, LineSegment2 line);
RaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 aabbMins, Vec2 aabbMaxs);
RaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, AABB2 aabb);

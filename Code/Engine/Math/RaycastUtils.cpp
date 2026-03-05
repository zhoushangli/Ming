#include "Engine/Math/RaycastUtils.hpp"

#include "Engine/Math/MathUtils.hpp"

#include <math.h>

RaycastResult2D RaycastVsDisc2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 discCenter, float discRadius)
{
    RaycastResult2D result;

    Vec2 i = forwardNormal;
    Vec2 j = i.GetRotatedBy90Degrees();
    Vec2 startToCenter = discCenter - startPos;
    Vec2 endPos = startPos + (forwardNormal * maxDist);

    // --- Whether raycast could hit disc ---
    float SCj = DotProduct2D(startToCenter, j);
    if (SCj > discRadius || SCj < -discRadius)
    {
        return result;
    }

    // --- Whether raycast is too far or too close ---
    float SCi = DotProduct2D(startToCenter, i);
    if (SCi > maxDist + discRadius || SCi <= -discRadius)
    {
        return result;
    }

    // --- Whether raycast starts inside disc ---
    if (IsPointInsideDisc2D(startPos, discCenter, discRadius))
    {
        result.m_didImpact    = true;
        result.m_impactDist   = 0.f;
        result.m_impactPos    = startPos;
        result.m_impactNormal = -forwardNormal;
        return result;
    }

    // Calculate "adjustment" dist to come back from SCj via Pythagorean theorem
    float adjustmentDist = sqrtf((discRadius * discRadius) - (SCj * SCj));
    float impactDist = SCi - adjustmentDist;

    // Check if impact dist is too late or too early
    if (impactDist >= maxDist || impactDist <= 0.f)
    {
        return result;
    }

    // Raycast did impact
    result.m_didImpact    = true;
    result.m_impactDist   = impactDist;
    result.m_impactPos    = startPos + (forwardNormal * result.m_impactDist);
    result.m_impactNormal = (result.m_impactPos - discCenter).GetNormalized();

    return result;
}

RaycastResult2D RaycastVsDisc2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Disc2 disc)
{
    return RaycastVsDisc2D(startPos, forwardNormal, maxDist, disc.m_center, disc.m_radius);
}

RaycastResult2D RaycastVsLineSegments2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 lineStartPos, Vec2 lineEndPos)
{
    RaycastResult2D result;

    Vec2 i = forwardNormal;
    Vec2 j = i.GetRotatedBy90Degrees();

    Vec2 ra = lineStartPos - startPos;
    Vec2 rb = lineEndPos - startPos;

    float raxj = DotProduct2D(ra, j);
    float rbxj = DotProduct2D(rb, j);

    // --- Whether line segment is side to raycast ---
    if (raxj * rbxj > 0.f)
    {
        return result;
    }

    float raxi = DotProduct2D(ra, i);
    float rbxi = DotProduct2D(rb, i);

    // --- Whether line segment is too far or too close or parallel ---
    if (raxi > maxDist && rbxi > maxDist)
    {
        return result;
    }

    if (raxi < 0.f && rbxi < 0.f)
    {
        return result;
    }

    if (Abs(raxj - rbxj) < 1e-5)
    {
        return result;
    }

    float t = raxj / (raxj - rbxj);
    float impactDist = raxi + t * (rbxi - raxi);

    if (impactDist >= maxDist || impactDist <= 0.f)
    {
        return result;
    }
    
    result.m_didImpact = true;
    result.m_impactDist = impactDist;
    result.m_impactPos = lineStartPos + t * (lineEndPos - lineStartPos);
    result.m_impactNormal = (lineEndPos - lineStartPos).GetNormalized().GetRotatedBy90Degrees();

    if (DotProduct2D(result.m_impactNormal, forwardNormal) > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    return result;
}

RaycastResult2D RaycastVsLineSegments2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, LineSegment2 line)
{
    return RaycastVsLineSegments2D(startPos, forwardNormal, maxDist, line.m_start, line.m_end);
}

RaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, Vec2 aabbMins, Vec2 aabbMaxs)
{
    RaycastResult2D result;

    float tx1 = (aabbMins.x - startPos.x) / forwardNormal.x;
    float tx2 = (aabbMaxs.x - startPos.x) / forwardNormal.x;
    float ty1 = (aabbMins.y - startPos.y) / forwardNormal.y;
    float ty2 = (aabbMaxs.y - startPos.y) / forwardNormal.y;

    float tminX = Min(tx1, tx2);
    float tmaxX = Max(tx1, tx2);
    float tminY = Min(ty1, ty2);
    float tmaxY = Max(ty1, ty2);

    // --- Whether raycast starts inside disc ---
    if (IsPointInsideAABB2D(startPos, AABB2(aabbMins, aabbMaxs)))
    {
        result.m_didImpact = true;
        result.m_impactDist = 0.f;
        result.m_impactPos = startPos;
        result.m_impactNormal = -forwardNormal;
        return result;
    }

    if (tminX > tmaxY || tminY > tmaxX)
    {
        return result;
    }

    float impactDist = Max(tminX, tminY);

    if (impactDist >= maxDist || impactDist <= 0.f)
    {
        return result;
    }

    result.m_didImpact = true;
    result.m_impactDist = impactDist;
    result.m_impactPos = startPos + forwardNormal * impactDist;
    result.m_impactNormal = impactDist == tminX ? Vec2(1.f, 0.f) : Vec2(0.f, 1.f);

    if (DotProduct2D(result.m_impactNormal, forwardNormal) > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    return result;
}

RaycastResult2D RaycastVsAABB2D(Vec2 startPos, Vec2 forwardNormal, float maxDist, AABB2 aabb)
{
    return RaycastVsAABB2D(startPos, forwardNormal, maxDist, aabb.m_mins, aabb.m_maxs);
}

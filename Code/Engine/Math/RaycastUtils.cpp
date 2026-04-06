#include "Engine/Math/RaycastUtils.hpp"

#include "Engine/Math/MathUtils.hpp"

#include <math.h>
#include <utility>

RaycastResult2D RaycastVsDisc2D(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength, Vec2 discCenter, float discRadius)
{
    RaycastResult2D result(rayStart, rayForwardNormal, rayLength);

    Vec2 i = rayForwardNormal;
    Vec2 j = i.GetRotatedBy90Degrees();
    Vec2 startToCenter = discCenter - rayStart;
    Vec2 endPos = rayStart + (rayForwardNormal * rayLength);

    // --- Whether raycast could hit disc ---
    float SCj = DotProduct2D(startToCenter, j);
    if (SCj > discRadius || SCj < -discRadius)
    {
        return result;
    }

    // --- Whether raycast is too far or too close ---
    float SCi = DotProduct2D(startToCenter, i);
    if (SCi > rayLength + discRadius || SCi <= -discRadius)
    {
        return result;
    }

    // --- Whether raycast starts inside disc ---
    if (IsPointInsideDisc2D(rayStart, discCenter, discRadius))
    {
        result.m_didImpact = true;
        result.m_impactDist = 0.f;
        result.m_impactPos = rayStart;
        result.m_impactNormal = -rayForwardNormal;
        return result;
    }

    // Calculate "adjustment" dist to come back from SCj via Pythagorean theorem
    float adjustmentDist = sqrtf((discRadius * discRadius) - (SCj * SCj));
    float impactDist = SCi - adjustmentDist;

    // Check if impact dist is too late or too early
    if (impactDist >= rayLength || impactDist <= 0.f)
    {
        return result;
    }

    // Raycast did impact
    result.m_didImpact = true;
    result.m_impactDist = impactDist;
    result.m_impactPos = rayStart + (rayForwardNormal * result.m_impactDist);
    result.m_impactNormal = (result.m_impactPos - discCenter).GetNormalized();

    return result;
}

RaycastResult2D RaycastVsDisc2D(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength, Disc2 disc)
{
    return RaycastVsDisc2D(rayStart, rayForwardNormal, rayLength, disc.m_center, disc.m_radius);
}

RaycastResult2D RaycastVsLineSegments2D(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength, Vec2 lineStartPos, Vec2 lineEndPos)
{
    RaycastResult2D result(rayStart, rayForwardNormal, rayLength);

    Vec2 i = rayForwardNormal;
    Vec2 j = i.GetRotatedBy90Degrees();

    Vec2 ra = lineStartPos - rayStart;
    Vec2 rb = lineEndPos - rayStart;

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
    if (raxi > rayLength && rbxi > rayLength)
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

    if (impactDist >= rayLength || impactDist <= 0.f)
    {
        return result;
    }

    result.m_didImpact = true;
    result.m_impactDist = impactDist;
    result.m_impactPos = lineStartPos + t * (lineEndPos - lineStartPos);
    result.m_impactNormal = (lineEndPos - lineStartPos).GetNormalized().GetRotatedBy90Degrees();

    if (DotProduct2D(result.m_impactNormal, rayForwardNormal) > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    return result;
}

RaycastResult2D RaycastVsLineSegments2D(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength, LineSegment2 line)
{
    return RaycastVsLineSegments2D(rayStart, rayForwardNormal, rayLength, line.m_start, line.m_end);
}

RaycastResult2D RaycastVsAABB2D(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength, Vec2 aabbMins, Vec2 aabbMaxs)
{
    RaycastResult2D result(rayStart, rayForwardNormal, rayLength);

    float tx1 = (aabbMins.x - rayStart.x) / rayForwardNormal.x;
    float tx2 = (aabbMaxs.x - rayStart.x) / rayForwardNormal.x;
    float ty1 = (aabbMins.y - rayStart.y) / rayForwardNormal.y;
    float ty2 = (aabbMaxs.y - rayStart.y) / rayForwardNormal.y;

    float tminX = Min(tx1, tx2);
    float tmaxX = Max(tx1, tx2);
    float tminY = Min(ty1, ty2);
    float tmaxY = Max(ty1, ty2);

    // --- Whether raycast starts inside disc ---
    if (IsPointInsideAABB2D(rayStart, AABB2(aabbMins, aabbMaxs)))
    {
        result.m_didImpact = true;
        result.m_impactDist = 0.f;
        result.m_impactPos = rayStart;
        result.m_impactNormal = -rayForwardNormal;
        return result;
    }

    if (tminX > tmaxY || tminY > tmaxX)
    {
        return result;
    }

    float impactDist = Max(tminX, tminY);

    if (impactDist >= rayLength || impactDist <= 0.f)
    {
        return result;
    }

    result.m_didImpact = true;
    result.m_impactDist = impactDist;
    result.m_impactPos = rayStart + rayForwardNormal * impactDist;
    result.m_impactNormal = impactDist == tminX ? Vec2(1.f, 0.f) : Vec2(0.f, 1.f);

    if (DotProduct2D(result.m_impactNormal, rayForwardNormal) > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    return result;
}

RaycastResult2D RaycastVsAABB2D(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength, AABB2 aabb)
{
    return RaycastVsAABB2D(rayStart, rayForwardNormal, rayLength, aabb.m_mins, aabb.m_maxs);
}

RaycastResult3D RaycastVsAABB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, AABB3 box)
{
    RaycastResult3D result(rayStart, rayForwardNormal, rayLength);

    bool isInside =
        rayStart.x >= box.m_mins.x && rayStart.x <= box.m_maxs.x &&
        rayStart.y >= box.m_mins.y && rayStart.y <= box.m_maxs.y &&
        rayStart.z >= box.m_mins.z && rayStart.z <= box.m_maxs.z;
    if (isInside)
    {
        result.m_didImpact = true;
        result.m_impactDist = 0.f;
        result.m_impactPos = rayStart;
        result.m_impactNormal = -rayForwardNormal;
        return result;
    }

    float tMin = 0.f;
    float tMax = rayLength;
    Vec3 impactNormal = Vec3::ZERO;

    auto UpdateSlab = [&](float start, float dir, float minValue, float maxValue, Vec3 const &minNormal, Vec3 const &maxNormal) -> bool
    {
        float const epsilon = 1e-8f;
        if (Abs(dir) <= epsilon)
        {
            return start >= minValue && start <= maxValue;
        }

        float t0 = (minValue - start) / dir;
        float t1 = (maxValue - start) / dir;
        Vec3 nearNormal = minNormal;
        Vec3 farNormal = maxNormal;
        if (t0 > t1)
        {
            std::swap(t0, t1);
            std::swap(nearNormal, farNormal);
        }

        if (t0 > tMin)
        {
            tMin = t0;
            impactNormal = nearNormal;
        }

        tMax = Min(tMax, t1);
        return tMin <= tMax;
    };

    if (!UpdateSlab(rayStart.x, rayForwardNormal.x, box.m_mins.x, box.m_maxs.x, Vec3(-1.f, 0.f, 0.f), Vec3(1.f, 0.f, 0.f)))
    {
        return result;
    }

    if (!UpdateSlab(rayStart.y, rayForwardNormal.y, box.m_mins.y, box.m_maxs.y, Vec3(0.f, -1.f, 0.f), Vec3(0.f, 1.f, 0.f)))
    {
        return result;
    }

    if (!UpdateSlab(rayStart.z, rayForwardNormal.z, box.m_mins.z, box.m_maxs.z, Vec3(0.f, 0.f, -1.f), Vec3(0.f, 0.f, 1.f)))
    {
        return result;
    }

    if (tMin < 0.f || tMin > rayLength)
    {
        return result;
    }

    result.m_didImpact = true;
    result.m_impactDist = tMin;
    result.m_impactPos = rayStart + rayForwardNormal * tMin;
    result.m_impactNormal = impactNormal;

    if (DotProduct3D(result.m_impactNormal, rayForwardNormal) > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    return result;
}

RaycastResult3D RaycastVsSphere3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 sphereCenter, float sphereRadius)
{
    RaycastResult3D result(rayStart, rayForwardNormal, rayLength);

    float startToCenterDistSquared = GetDistanceSquared3D(rayStart, sphereCenter);
    if (startToCenterDistSquared < (sphereRadius * sphereRadius))
    {
        result.m_didImpact = true;
        result.m_impactDist = 0.f;
        result.m_impactPos = rayStart;
        result.m_impactNormal = -rayForwardNormal;
        return result;
    }

    Vec3 startToCenter = rayStart - sphereCenter;
    float b = DotProduct3D(startToCenter, rayForwardNormal);
    float c = DotProduct3D(startToCenter, startToCenter) - sphereRadius * sphereRadius;
    float discriminant = b * b - c;
    if (discriminant < 0.f)
    {
        return result;
    }

    float impactDist = -b - sqrtf(discriminant);
    if (impactDist < 0.f || impactDist > rayLength)
    {
        return result;
    }

    result.m_didImpact = true;
    result.m_impactDist = impactDist;
    result.m_impactPos = rayStart + rayForwardNormal * impactDist;
    result.m_impactNormal = (result.m_impactPos - sphereCenter).GetNormalized();

    if (DotProduct3D(result.m_impactNormal, rayForwardNormal) > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    return result;
}

RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec2 const &centerXY, FloatRange const &minMaxZ, float radiusXY)
{
    RaycastResult3D result(rayStart, rayForwardNormal, rayLength);

    //--------------------------------------
    // XY interval
    //--------------------------------------
    float xyEnter = -1e9f;
    float xyExit = 1e9f;

    Vec2 startXY(rayStart.x, rayStart.y);
    Vec2 forwardXY(rayForwardNormal.x, rayForwardNormal.y);

    float forwardXYLength = forwardXY.GetLength();
    if (forwardXYLength < 1e-9f)
    {
        if (!IsPointInsideDisc2D(startXY, centerXY, radiusXY))
        {
            return result;
        }
    }
    else
    {
        Vec2 i = forwardXY / forwardXYLength;
        Vec2 j = i.GetRotatedBy90Degrees();
        Vec2 startToCenter = centerXY - startXY;

        float SCj = DotProduct2D(startToCenter, j);
        if (SCj > radiusXY || SCj < -radiusXY)
        {
            return result;
        }

        float SCi = DotProduct2D(startToCenter, i);

        if (SCi > rayLength * forwardXYLength + radiusXY || SCi < -radiusXY)
        {
            return result;
        }

        float adjustmentDist = sqrtf(radiusXY * radiusXY - SCj * SCj);

        float enterDistXY = SCi - adjustmentDist;
        float exitDistXY = SCi + adjustmentDist;

        xyEnter = enterDistXY / forwardXYLength;
        xyExit = exitDistXY / forwardXYLength;
    }

    //--------------------------------------
    // Z interval
    //--------------------------------------
    float zEnter = -1e9f;
    float zExit = 1e9f;
    if (Abs(rayForwardNormal.z) < 1e-9f)
    {
        if (rayStart.z < minMaxZ.m_min || rayStart.z > minMaxZ.m_max)
        {
            return result;
        }
    }
    else
    {
        float tz0 = (minMaxZ.m_min - rayStart.z) / rayForwardNormal.z;
        float tz1 = (minMaxZ.m_max - rayStart.z) / rayForwardNormal.z;
        zEnter = Min(tz0, tz1);
        zExit = Max(tz0, tz1);
    }

    float enterTime = Max(0.f, Max(xyEnter, zEnter));
    float exitTime = Min(rayLength, Min(xyExit, zExit));

    if (enterTime > exitTime)
    {
        return result;
    }

    result.m_didImpact = true;
    result.m_impactDist = enterTime;
    result.m_impactPos = rayStart + rayForwardNormal * enterTime;

    float const zNormalEpsilon = 1e-4f;
    if (Abs(result.m_impactPos.z - minMaxZ.m_min) <= zNormalEpsilon)
    {
        result.m_impactNormal = Vec3(0.f, 0.f, -1.f);
    }
    else if (Abs(result.m_impactPos.z - minMaxZ.m_max) <= zNormalEpsilon)
    {
        result.m_impactNormal = Vec3(0.f, 0.f, 1.f);
    }
    else
    {
        Vec2 sideNormalXY = (Vec2(result.m_impactPos.x, result.m_impactPos.y) - centerXY).GetNormalized();
        result.m_impactNormal = Vec3(sideNormalXY.x, sideNormalXY.y, 0.f);
    }

    if (DotProduct3D(result.m_impactNormal, rayForwardNormal) > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    return result;
}
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

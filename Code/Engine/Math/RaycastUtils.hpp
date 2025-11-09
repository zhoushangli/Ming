#pragma once

#include "Engine/Math/Vec2.hpp"

struct OBB2
{
public:
	OBB2() = default;
	OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions);
    OBB2(Vec2 const& center, Vec2 const& halfDimensions, float orientationDegree);

    void GetCornerPoints(Vec2* out_fourCornerWorldPositions) const;
    Vec2 GetLocalPosForWorldPos(Vec2 const& worldPos) const;
    Vec2 GetWorldPosForLocalPos(Vec2 const& localPos) const;
    void RotateAboutCenter(float rotationDeltaDegrees);

public:
    Vec2 m_center;
    Vec2 m_iBasisNormal;
    Vec2 m_halfDimensions;
};


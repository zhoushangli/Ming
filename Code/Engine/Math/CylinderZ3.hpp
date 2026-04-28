#pragma once

#include "Vec2.hpp"
#include "Vec3.hpp"
#include "Engine/Math/FloatRange.hpp"

class CylinderZ3
{
public:
    CylinderZ3() = default;
    CylinderZ3(Vec3 const &start, float height, float radius);
    CylinderZ3(Vec2 const &centerXY, FloatRange const &minMaxZ, float radius);
    ~CylinderZ3() = default;

    Vec3 const GetNearestPoint(Vec3 const &point) const;

public:
    Vec2 m_centerXY;
    float m_radius;
    FloatRange m_minMaxZ;
};
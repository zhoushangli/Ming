#pragma once

#include "Vec3.hpp"
#include "Engine/Math/FloatRange.hpp"

class CylinderZ3
{
public:
    CylinderZ3() = default;
    CylinderZ3(Vec3 const &start, float height, float radius);
    ~CylinderZ3() = default;

    Vec3 const GetNearestPoint(Vec3 const &point) const;
    FloatRange GetMinMaxZ() const;

public:
    Vec3 m_start;
    float m_height;
    float m_radius;
};
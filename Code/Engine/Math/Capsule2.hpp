#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/LineSegment2.hpp"

struct Capsule2
{
public:
    void Translate(const Vec2& translation);
    void SetCenter(const Vec2& newCenter);
    void RotateAboutCenter(float rotationDeltaDegrees);

private:
    LineSegment2 m_bone;
    float m_radius = 0.f;
};


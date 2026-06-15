#pragma once

#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/LineSegment2.hpp"

struct Capsule2
{
public:
	Capsule2() = default;
	Capsule2(const Vec2& start, const Vec2& end, float radius);

    void Translate(const Vec2& translation);
    void SetCenter(const Vec2& newCenter);
    void RotateAboutCenter(float rotationDeltaDegrees);

public:
    LineSegment2 m_bone;
    float m_radius = 0.f;
};



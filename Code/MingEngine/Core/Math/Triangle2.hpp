#pragma once

#include "MingEngine/Core/Math/Vec2.hpp"

struct Triangle2
{
public:
	Triangle2() = default;
	Triangle2(const Vec2& pointA, const Vec2& pointB, const Vec2& pointC);
    void Translate(const Vec2& translation);

public:
    Vec2 m_pointsCounterClockwise[3];
};



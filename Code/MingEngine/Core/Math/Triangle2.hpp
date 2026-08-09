#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

struct Triangle2
{
public:
	Triangle2() = default;
	Triangle2(const Vector2& pointA, const Vector2& pointB, const Vector2& pointC);
	void Translate(const Vector2& translation);

public:
	Vector2 m_pointsCounterClockwise[3];
};

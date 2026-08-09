#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

struct LineSegment2
{
public:
	LineSegment2() = default;
	LineSegment2(const Vector2& start, const Vector2& end);

	void Translate(const Vector2& translation);
	void SetCenter(const Vector2& newCenter);
	void RotateAboutCenter(float rotationDeltaDegrees);

public:
	Vector2 m_start;
	Vector2 m_end;
};

#pragma once

#include "MingEngine/Core/Math/LineSegment2.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"

struct Capsule2
{
public:
	Capsule2() = default;
	Capsule2(const Vector2& start, const Vector2& end, float radius);

	void Translate(const Vector2& translation);
	void SetCenter(const Vector2& newCenter);
	void RotateAboutCenter(float rotationDeltaDegrees);

public:
	LineSegment2 m_bone;
	float        m_radius = 0.f;
};

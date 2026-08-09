#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

struct Disc2
{
public:
	Disc2() = default;
	Disc2(const Vector2& center, float radius);
	Disc2(float centerX, float centerY, float radius);

	void Translate(const Vector2& translation);
	void SetCenter(const Vector2& newCenter);

public:
	Vector2 m_center = Vector2::Zero;
	float   m_radius = 0.f;
};

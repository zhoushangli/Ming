#pragma once

#include "Engine/Math/Vec2.hpp"

struct Disc2
{
public:
	Disc2() = default;
	Disc2(const Vec2& center, float radius);
    Disc2(float centerX, float centerY, float radius);

    void Translate(const Vec2& translation);
    void SetCenter(const Vec2& newCenter);

public:
    Vec2 m_center = Vec2::kZero;
    float m_radius = 0.f;
};


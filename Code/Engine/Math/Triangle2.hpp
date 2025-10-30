#pragma once

#include "Engine/Math/Vec2.hpp"

struct Triangle2
{
public:
    void Translate(const Vec2& translation);

public:
    Vec2 m_pointsCounterClockwise[3];
};


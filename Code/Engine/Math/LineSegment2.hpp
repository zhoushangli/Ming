#pragma once

#include "Engine/Math/Vec2.hpp"

struct LineSegment2
{
public:
    void Translate(const Vec2& translation);
    void SetCenter(const Vec2& newCenter);
    void RotateAboutCenter(float rotationDeltaDegrees);
    
public:
    Vec2 m_start;
    Vec2 m_end;
};


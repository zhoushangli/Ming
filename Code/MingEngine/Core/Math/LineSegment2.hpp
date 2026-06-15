#pragma once

#include "MingEngine/Core/Math/Vec2.hpp"

struct LineSegment2
{
public:
	LineSegment2() = default;
	LineSegment2(const Vec2& start, const Vec2& end);

    void Translate(const Vec2& translation);
    void SetCenter(const Vec2& newCenter);
    void RotateAboutCenter(float rotationDeltaDegrees);
    
public:
    Vec2 m_start;
    Vec2 m_end;
};



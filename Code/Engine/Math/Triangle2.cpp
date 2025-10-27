#include "Engine/Math/Triangle2.hpp"

void Triangle2::Translate(const Vec2& translation)
{
    m_pointsCounterClockwise[0] += translation;
    m_pointsCounterClockwise[1] += translation;
    m_pointsCounterClockwise[2] += translation;
}



#include "MingEngine/Engine/Math/Triangle2.hpp"

Triangle2::Triangle2(const Vec2& pointA, const Vec2& pointB, const Vec2& pointC)
{
	m_pointsCounterClockwise[0] = pointA;
	m_pointsCounterClockwise[1] = pointB;
	m_pointsCounterClockwise[2] = pointC;
}

void Triangle2::Translate(const Vec2& translation)
{
    m_pointsCounterClockwise[0] += translation;
    m_pointsCounterClockwise[1] += translation;
    m_pointsCounterClockwise[2] += translation;
}



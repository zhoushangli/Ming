#include "MingEngine/Core/Math/Triangle2.hpp"

Triangle2::Triangle2(const Vector2& pointA, const Vector2& pointB, const Vector2& pointC)
{
	m_pointsCounterClockwise[0] = pointA;
	m_pointsCounterClockwise[1] = pointB;
	m_pointsCounterClockwise[2] = pointC;
}

void Triangle2::Translate(const Vector2& translation)
{
	m_pointsCounterClockwise[0] += translation;
	m_pointsCounterClockwise[1] += translation;
	m_pointsCounterClockwise[2] += translation;
}

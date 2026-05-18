#include "Engine/Math/Triangle3.hpp"

Triangle3::Triangle3(Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC)
{
	m_pointsCounterClockwise[0] = pointA;
	m_pointsCounterClockwise[1] = pointB;
	m_pointsCounterClockwise[2] = pointC;
}

void Triangle3::Translate(Vec3 const& translation)
{
	m_pointsCounterClockwise[0] += translation;
	m_pointsCounterClockwise[1] += translation;
	m_pointsCounterClockwise[2] += translation;
}

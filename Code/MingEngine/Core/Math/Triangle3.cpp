#include "MingEngine/Core/Math/Triangle3.hpp"

Triangle3::Triangle3(Vector3 const& pointA, Vector3 const& pointB, Vector3 const& pointC)
{
	m_pointsCounterClockwise[0] = pointA;
	m_pointsCounterClockwise[1] = pointB;
	m_pointsCounterClockwise[2] = pointC;
}

void Triangle3::Translate(Vector3 const& translation)
{
	m_pointsCounterClockwise[0] += translation;
	m_pointsCounterClockwise[1] += translation;
	m_pointsCounterClockwise[2] += translation;
}

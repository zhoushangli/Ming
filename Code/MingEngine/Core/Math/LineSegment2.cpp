#include "MingEngine/Core/Math/LineSegment2.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"

LineSegment2::LineSegment2(const Vector2& start, const Vector2& end) : m_start(start), m_end(end) {}

void LineSegment2::Translate(const Vector2& translation)
{
	m_start += translation;
	m_end += translation;
}

void LineSegment2::SetCenter(const Vector2& newCenter)
{
	Vector2 center = (m_start + m_end) * 0.5f;
	Vector2 offset = newCenter - center;
	m_start += offset;
	m_end += offset;
}

void LineSegment2::RotateAboutCenter(float rotationDeltaDegrees)
{
	Vector2 center   = (m_start + m_end) * 0.5f;
	Vector2 dirStart = m_start - center;
	Vector2 dirEnd   = m_end - center;
	dirStart         = dirStart.GetRotatedByDegrees(rotationDeltaDegrees);
	dirEnd           = dirEnd.GetRotatedByDegrees(rotationDeltaDegrees);
	m_start          = center + dirStart;
	m_end            = center + dirEnd;
}

#include "MingEngine/Engine/Math/LineSegment2.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"

LineSegment2::LineSegment2(const Vec2& start, const Vec2& end) : 
    m_start(start),
	m_end(end)
{
}

void LineSegment2::Translate(const Vec2& translation)
{
    m_start += translation;
    m_end += translation;
}

void LineSegment2::SetCenter(const Vec2& newCenter)
{
    Vec2 center = (m_start + m_end) * 0.5f;
    Vec2 offset = newCenter - center;
    m_start += offset;
    m_end += offset;
}

void LineSegment2::RotateAboutCenter(float rotationDeltaDegrees)
{
    Vec2 center = (m_start + m_end) * 0.5f;
    Vec2 dirStart = m_start - center;
    Vec2 dirEnd = m_end - center;
    dirStart = dirStart.GetRotatedByDegrees(rotationDeltaDegrees);
    dirEnd = dirEnd.GetRotatedByDegrees(rotationDeltaDegrees);
    m_start = center + dirStart;
    m_end = center + dirEnd;
}



#include "Engine/Math/AABB2.hpp"

#include "Engine/Math/MathUtils.hpp"

const AABB2 AABB2::Zero          = AABB2(0.f, 0.f, 0.f, 0.f);
const AABB2 AABB2::Unit          = AABB2(0.f, 0.f, 1.f, 1.f);
const AABB2 AABB2::kCenteredUnit = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);

AABB2::AABB2(const Vec2& mins, const Vec2& maxs) : m_mins(mins), m_maxs(maxs) {}

AABB2::AABB2(float minX, float minY, float maxX, float maxY) : m_mins(minX, minY), m_maxs(maxX, maxY) {}

bool AABB2::IsPointInside(const Vec2& point) const
{
	return (point.x >= m_mins.x && point.x <= m_maxs.x && point.y >= m_mins.y && point.y <= m_maxs.y);
}

float const AABB2::GetWidth() const { return m_maxs.x - m_mins.x; }

float const AABB2::GetHeight() const { return m_maxs.y - m_mins.y; }

Vec2 const AABB2::GetCenter() const { return (m_mins + m_maxs) * 0.5f; }

Vec2 const AABB2::GetDimensions() const { return m_maxs - m_mins; }

Vec2 const AABB2::GetNearestPoint(const Vec2& point) const
{
	return Vec2(GetClamped(point.x, m_mins.x, m_maxs.x), GetClamped(point.y, m_mins.y, m_maxs.y));
}

Vec2 const AABB2::GetPointAtUV(const Vec2& uv) const
{
	return Vec2(Interpolate(m_mins.x, m_maxs.x, uv.x), Interpolate(m_mins.y, m_maxs.y, uv.y));
}

Vec2 const AABB2::GetUVForPoint(const Vec2& point) const
{
	return Vec2(
		GetFractionWithinRange(point.x, m_mins.x, m_maxs.x),
		GetFractionWithinRange(point.y, m_mins.y, m_maxs.y)
	);
}

void AABB2::Translate(const Vec2& translation)
{
	m_mins += translation;
	m_maxs += translation;
}

void AABB2::SetCenter(const Vec2& newCenter)
{
	Vec2 dimensions = GetDimensions();
	m_mins          = newCenter - (dimensions * 0.5f);
	m_maxs          = m_mins + dimensions;
}

void AABB2::SetDimensions(const Vec2& newDimensions)
{
	Vec2 center = GetCenter();
	m_mins      = center - (newDimensions * 0.5f);
	m_maxs      = m_mins + newDimensions;
}

void AABB2::Shrink(float uniformAmount) { Shrink(uniformAmount, uniformAmount); }

void AABB2::Shrink(float amountX, float amountY)
{
	Vec2 const  dimensions = GetDimensions();
	float const maxShrinkX = dimensions.x * 0.5f;
	float const maxShrinkY = dimensions.y * 0.5f;

	amountX = GetClamped(amountX, 0.f, maxShrinkX);
	amountY = GetClamped(amountY, 0.f, maxShrinkY);

	m_mins.x += amountX;
	m_maxs.x -= amountX;
	m_mins.y += amountY;
	m_maxs.y -= amountY;
}

void AABB2::StretchToIncludePoint(const Vec2& point)
{
	if (point.x < m_mins.x)
	{
		m_mins.x = point.x;
	}
	else if (point.x > m_maxs.x)
	{
		m_maxs.x = point.x;
	}

	if (point.y < m_mins.y)
	{
		m_mins.y = point.y;
	}
	else if (point.y > m_maxs.y)
	{
		m_maxs.y = point.y;
	}
}

bool AABB2::operator==(const AABB2& other) const
{
	return m_mins.x == other.m_mins.x && m_mins.y == other.m_mins.y && m_maxs.x == other.m_maxs.x
		   && m_maxs.y == other.m_maxs.y;
}

AABB2& AABB2::operator=(const AABB2& other)
{
	if (this != &other)
	{
		m_mins = other.m_mins;
		m_maxs = other.m_maxs;
	}
	return *this;
}

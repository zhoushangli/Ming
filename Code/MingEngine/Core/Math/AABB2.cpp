#include "MingEngine/Core/Math/AABB2.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"

using namespace Math;

const AABB2 AABB2::Zero          = AABB2(0.f, 0.f, 0.f, 0.f);
const AABB2 AABB2::Unit          = AABB2(0.f, 0.f, 1.f, 1.f);
const AABB2 AABB2::kCenteredUnit = AABB2(-0.5f, -0.5f, 0.5f, 0.5f);

AABB2::AABB2(const Vector2& mins, const Vector2& maxs) : m_mins(mins), m_maxs(maxs) {}

AABB2::AABB2(float minX, float minY, float maxX, float maxY) : m_mins(minX, minY), m_maxs(maxX, maxY) {}

bool AABB2::IsPointInside(const Vector2& point) const
{
	return (point.x >= m_mins.x && point.x <= m_maxs.x && point.y >= m_mins.y && point.y <= m_maxs.y);
}

float const AABB2::GetWidth() const { return m_maxs.x - m_mins.x; }

float const AABB2::GetHeight() const { return m_maxs.y - m_mins.y; }

Vector2 const AABB2::GetCenter() const { return (m_mins + m_maxs) * 0.5f; }

Vector2 const AABB2::GetDimensions() const { return m_maxs - m_mins; }

Vector2 const AABB2::GetNearestPoint(const Vector2& point) const
{
	return Vector2(GetClamped(point.x, m_mins.x, m_maxs.x), GetClamped(point.y, m_mins.y, m_maxs.y));
}

Vector2 const AABB2::GetPointAtUV(const Vector2& uv) const
{
	return Vector2(Interpolate(m_mins.x, m_maxs.x, uv.x), Interpolate(m_mins.y, m_maxs.y, uv.y));
}

Vector2 const AABB2::GetUVForPoint(const Vector2& point) const
{
	return Vector2(
		GetFractionWithinRange(point.x, m_mins.x, m_maxs.x),
		GetFractionWithinRange(point.y, m_mins.y, m_maxs.y));
}

void AABB2::Translate(const Vector2& translation)
{
	m_mins += translation;
	m_maxs += translation;
}

void AABB2::SetCenter(const Vector2& newCenter)
{
	Vector2 dimensions = GetDimensions();
	m_mins             = newCenter - (dimensions * 0.5f);
	m_maxs             = m_mins + dimensions;
}

void AABB2::SetDimensions(const Vector2& newDimensions)
{
	Vector2 center = GetCenter();
	m_mins         = center - (newDimensions * 0.5f);
	m_maxs         = m_mins + newDimensions;
}

void AABB2::Shrink(float uniformAmount) { Shrink(uniformAmount, uniformAmount); }

void AABB2::Shrink(float amountX, float amountY)
{
	Vector2 const dimensions = GetDimensions();
	float const   maxShrinkX = dimensions.x * 0.5f;
	float const   maxShrinkY = dimensions.y * 0.5f;

	amountX = GetClamped(amountX, 0.f, maxShrinkX);
	amountY = GetClamped(amountY, 0.f, maxShrinkY);

	m_mins.x += amountX;
	m_maxs.x -= amountX;
	m_mins.y += amountY;
	m_maxs.y -= amountY;
}

void AABB2::StretchToIncludePoint(const Vector2& point)
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

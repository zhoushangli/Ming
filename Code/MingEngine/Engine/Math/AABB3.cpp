#include "MingEngine/Engine/Math/AABB3.hpp"

#include "MingEngine/Engine/Math/MathUtils.hpp"

const AABB3 AABB3::Zero          = AABB3(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
const AABB3 AABB3::Unit          = AABB3(0.f, 0.f, 0.f, 1.f, 1.f, 1.f);
const AABB3 AABB3::kCenteredUnit = AABB3(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);

AABB3::AABB3(const Vec3& mins, const Vec3& maxs) : m_mins(mins), m_maxs(maxs) {}

AABB3::AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
	: m_mins(minX, minY, minZ), m_maxs(maxX, maxY, maxZ)
{
}

bool AABB3::IsPointInside(const Vec3& point) const
{
	return (
		point.x >= m_mins.x && point.x <= m_maxs.x && point.y >= m_mins.y && point.y <= m_maxs.y && point.z >= m_mins.z
		&& point.z <= m_maxs.z
	);
}

Vec3 const AABB3::GetCenter() const { return (m_mins + m_maxs) * 0.5f; }

Vec3 const AABB3::GetDimensions() const { return m_maxs - m_mins; }

Vec3 const AABB3::GetNearestPoint(const Vec3& point) const
{
	return Vec3(
		GetClamped(point.x, m_mins.x, m_maxs.x),
		GetClamped(point.y, m_mins.y, m_maxs.y),
		GetClamped(point.z, m_mins.z, m_maxs.z)
	);
}

Vec3 const AABB3::GetPointAtUV(const Vec3& uvw) const
{
	return Vec3(
		Interpolate(m_mins.x, m_maxs.x, uvw.x),
		Interpolate(m_mins.y, m_maxs.y, uvw.y),
		Interpolate(m_mins.z, m_maxs.z, uvw.z)
	);
}

Vec3 const AABB3::GetUVForPoint(const Vec3& point) const
{
	return Vec3(
		GetFractionWithinRange(point.x, m_mins.x, m_maxs.x),
		GetFractionWithinRange(point.y, m_mins.y, m_maxs.y),
		GetFractionWithinRange(point.z, m_mins.z, m_maxs.z)
	);
}

void AABB3::Translate(const Vec3& translation)
{
	m_mins += translation;
	m_maxs += translation;
}

void AABB3::SetCenter(const Vec3& newCenter)
{
	Vec3 dimensions = GetDimensions();
	m_mins          = newCenter - (dimensions * 0.5f);
	m_maxs          = m_mins + dimensions;
}

void AABB3::SetDimensions(const Vec3& newDimensions)
{
	Vec3 center = GetCenter();
	m_mins      = center - (newDimensions * 0.5f);
	m_maxs      = m_mins + newDimensions;
}

void AABB3::StretchToIncludePoint(const Vec3& point)
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

	if (point.z < m_mins.z)
	{
		m_mins.z = point.z;
	}
	else if (point.z > m_maxs.z)
	{
		m_maxs.z = point.z;
	}
}

bool AABB3::operator==(const AABB3& other) const
{
	return m_mins.x == other.m_mins.x && m_mins.y == other.m_mins.y && m_mins.z == other.m_mins.z
		   && m_maxs.x == other.m_maxs.x && m_maxs.y == other.m_maxs.y && m_maxs.z == other.m_maxs.z;
}

AABB3& AABB3::operator=(const AABB3& other)
{
	if (this == &other)
	{
		return *this;
	}

	m_mins = other.m_mins;
	m_maxs = other.m_maxs;
	return *this;
}

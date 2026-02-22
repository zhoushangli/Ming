#include "Engine/Math/AABB3.hpp"

AABB3::AABB3(const Vec3& mins, const Vec3& maxs)
	: m_mins(mins)
	, m_maxs(maxs)
{
}

AABB3::AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
	: m_mins(minX, minY, minZ)
	, m_maxs(maxX, maxY, maxZ)
{
}

bool AABB3::operator==(const AABB3& other) const
{
	return m_mins.x == other.m_mins.x
		&& m_mins.y == other.m_mins.y
		&& m_mins.z == other.m_mins.z
		&& m_maxs.x == other.m_maxs.x
		&& m_maxs.y == other.m_maxs.y
		&& m_maxs.z == other.m_maxs.z;
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
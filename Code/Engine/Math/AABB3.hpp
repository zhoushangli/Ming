#pragma once

#include "Engine/Math/Vec3.hpp"

class AABB3
{
public:
	AABB3() = default;
	AABB3(const AABB3& copy) = default;
	~AABB3() = default;

	explicit AABB3(const Vec3& mins, const Vec3& maxs);
	explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

	bool operator==(const AABB3& other) const;
	AABB3& operator=(const AABB3& other);

public:
	Vec3 m_mins;
	Vec3 m_maxs;
};
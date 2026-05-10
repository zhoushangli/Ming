#pragma once

#include "Engine/Math/Vec3.hpp"

class AABB3
{
public:
	AABB3()                  = default;
	AABB3(const AABB3& copy) = default;
	~AABB3()                 = default;

	explicit AABB3(const Vec3& mins, const Vec3& maxs);
	explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

	bool       IsPointInside(const Vec3& point) const;
	Vec3 const GetCenter() const;
	Vec3 const GetDimensions() const;
	Vec3 const GetNearestPoint(const Vec3& point) const;
	Vec3 const GetPointAtUV(const Vec3& uvw) const;
	Vec3 const GetUVForPoint(const Vec3& point) const;

	void Translate(const Vec3& translation);
	void SetCenter(const Vec3& newCenter);
	void SetDimensions(const Vec3& newDimensions);
	void StretchToIncludePoint(const Vec3& point);

	bool   operator==(const AABB3& other) const;
	AABB3& operator=(const AABB3& other);

public:
	Vec3 m_mins;
	Vec3 m_maxs;

	static const AABB3 Zero;
	static const AABB3 kUnit;
	static const AABB3 kCenteredUnit;
};

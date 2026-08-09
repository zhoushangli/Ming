#pragma once

#include "MingEngine/Core/Math/Vector3.hpp"

class AABB3
{
public:
	AABB3()                  = default;
	AABB3(const AABB3& copy) = default;
	~AABB3()                 = default;

	explicit AABB3(const Vector3& mins, const Vector3& maxs);
	explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

	bool          IsPointInside(const Vector3& point) const;
	Vector3 const GetCenter() const;
	Vector3 const GetDimensions() const;
	Vector3 const GetNearestPoint(const Vector3& point) const;
	Vector3 const GetPointAtUV(const Vector3& uvw) const;
	Vector3 const GetUVForPoint(const Vector3& point) const;

	void Translate(const Vector3& translation);
	void SetCenter(const Vector3& newCenter);
	void SetDimensions(const Vector3& newDimensions);
	void StretchToIncludePoint(const Vector3& point);

	bool   operator==(const AABB3& other) const;
	AABB3& operator=(const AABB3& other);

public:
	Vector3 m_mins;
	Vector3 m_maxs;

	static const AABB3 Zero;
	static const AABB3 Unit;
	static const AABB3 kCenteredUnit;
};

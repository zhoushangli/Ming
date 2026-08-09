#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

class AABB2
{
public:
	AABB2()                  = default;
	AABB2(const AABB2& copy) = default;
	~AABB2()                 = default;
	explicit AABB2(const Vector2& mins, const Vector2& maxs);
	explicit AABB2(float minX, float minY, float maxX, float maxY);

	// Methods to implement
	bool          IsPointInside(const Vector2& point) const;
	float const   GetWidth() const;
	float const   GetHeight() const;
	Vector2 const GetCenter() const;
	Vector2 const GetDimensions() const;
	Vector2 const GetNearestPoint(const Vector2& point) const;
	Vector2 const GetPointAtUV(const Vector2& uv) const;
	Vector2 const GetUVForPoint(const Vector2& point) const;

	void Translate(const Vector2& translation);
	void SetCenter(const Vector2& newCenter);
	void SetDimensions(const Vector2& newDimensions);
	void Shrink(float uniformAmount);
	void Shrink(float amountX, float amountY);
	void StretchToIncludePoint(const Vector2& point);

	bool   operator==(const AABB2& other) const;
	AABB2& operator=(const AABB2& other);

public:
	Vector2 m_mins;
	Vector2 m_maxs;

	static const AABB2 Zero;
	static const AABB2 Unit;
	static const AABB2 kCenteredUnit;
};

#pragma once

#include "Engine/Math/Vec2.hpp"

class AABB2
{
public:
	AABB2() = default;
	AABB2(const AABB2& copy) = default;
	~AABB2() = default;
	explicit AABB2(const Vec2& mins, const Vec2& maxs);
	explicit AABB2(float minX, float minY, float maxX, float maxY);

	// Methods to implement
	bool		IsPointInside(const Vec2& point) const;
	float const GetWidth() const;
	float const GetHeight() const;
	Vec2 const	GetCenter() const;
	Vec2 const	GetDimensions() const;
	Vec2 const	GetNearestPoint(const Vec2& point) const;
	Vec2 const	GetPointAtUV(const Vec2& uv) const;
	Vec2 const	GetUVForPoint(const Vec2& point) const;
	
	void		Translate(const Vec2& translation);
	void		SetCenter(const Vec2& newCenter);
	void		SetDimensions(const Vec2& newDimensions);
	void		Shrink(float uniformAmount);
	void		Shrink(float amountX, float amountY);
	void		StretchToIncludePoint(const Vec2& point);

	bool operator==(const AABB2& other) const;
	AABB2& operator=(const AABB2& other);

public:
	Vec2 m_mins;
	Vec2 m_maxs;

	static const AABB2 kZero;
	static const AABB2 kUnit;
    static const AABB2 kCenteredUnit;
};


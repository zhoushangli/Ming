#pragma once

#include "MingEngine/Core/Math/Vec3.hpp"

class Quad3
{
public:
	Quad3() = default;
	Quad3(Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC, Vec3 const& pointD)
		: m_points{ pointA, pointB, pointC, pointD }
	{
	}
	~Quad3() = default;

public:
	Vec3 m_points[4] = {};
};

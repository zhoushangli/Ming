#pragma once

#include "MingEngine/Core/Math/Vector3.hpp"

class Quad3
{
public:
	Quad3() = default;
	Quad3(Vector3 const& pointA, Vector3 const& pointB, Vector3 const& pointC, Vector3 const& pointD)
		: m_points{ pointA, pointB, pointC, pointD }
	{
	}
	~Quad3() = default;

public:
	Vector3 m_points[4] = {};
};

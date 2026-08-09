#pragma once

#include "MingEngine/Core/Math/Vector3.hpp"

struct Triangle3
{
public:
	Triangle3() = default;
	Triangle3(Vector3 const& pointA, Vector3 const& pointB, Vector3 const& pointC);
	void Translate(Vector3 const& translation);

public:
	Vector3 m_pointsCounterClockwise[3];
};

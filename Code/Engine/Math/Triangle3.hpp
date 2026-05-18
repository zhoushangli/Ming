#pragma once

#include "Engine/Math/Vec3.hpp"

struct Triangle3
{
public:
	Triangle3() = default;
	Triangle3(Vec3 const& pointA, Vec3 const& pointB, Vec3 const& pointC);
	void Translate(Vec3 const& translation);

public:
	Vec3 m_pointsCounterClockwise[3];
};

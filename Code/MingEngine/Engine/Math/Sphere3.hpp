#pragma once

#include "MingEngine/Engine/Math/Vec3.hpp"

class Sphere3
{
public:
	Sphere3() = default;
	Sphere3(Vec3 const& center, float radius);
	~Sphere3() = default;

	Vec3 const GetNearestPoint(Vec3 const& point) const;

public:
	Vec3  m_center = Vec3::Zero;
	float m_radius = 0.f;
};

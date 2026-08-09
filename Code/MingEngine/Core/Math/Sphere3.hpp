#pragma once

#include "MingEngine/Core/Math/Vector3.hpp"

class Sphere3
{
public:
	Sphere3() = default;
	Sphere3(Vector3 const& center, float radius);
	~Sphere3() = default;

	Vector3 const GetNearestPoint(Vector3 const& point) const;

public:
	Vector3 m_center = Vector3::Zero;
	float   m_radius = 0.f;
};

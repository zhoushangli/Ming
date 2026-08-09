#pragma once

#include "MingEngine/Core/Math/Vector3.hpp"

class Cone3
{
public:
	Cone3() = default;
	Cone3(Vector3 const& start, Vector3 const& end, float radius) : m_start(start), m_end(end), m_radius(radius) {}
	~Cone3() = default;

public:
	Vector3 m_start  = Vector3::Zero;
	Vector3 m_end    = Vector3::Zero;
	float   m_radius = 0.f;
};

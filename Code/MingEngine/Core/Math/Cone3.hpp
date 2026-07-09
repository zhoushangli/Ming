#pragma once

#include "MingEngine/Core/Math/Vec3.hpp"

class Cone3
{
public:
	Cone3() = default;
	Cone3(Vec3 const& start, Vec3 const& end, float radius) : m_start(start), m_end(end), m_radius(radius) {}
	~Cone3() = default;

public:
	Vec3  m_start  = Vec3::Zero;
	Vec3  m_end    = Vec3::Zero;
	float m_radius = 0.f;
};

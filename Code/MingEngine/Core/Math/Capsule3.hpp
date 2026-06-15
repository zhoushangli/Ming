#pragma once

#include "MingEngine/Core/Math/Vec3.hpp"

class Capsule3
{
public:
	Capsule3() = default;
	Capsule3(Vec3 const& start, Vec3 const& end, float radius);
	~Capsule3() = default;

public:
	Vec3  m_start  = Vec3::Zero;
	Vec3  m_end    = Vec3::Zero;
	float m_radius = 0.f;
};


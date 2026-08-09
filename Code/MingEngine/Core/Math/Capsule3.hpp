#pragma once

#include "MingEngine/Core/Math/Vector3.hpp"

class Capsule3
{
public:
	Capsule3() = default;
	Capsule3(Vector3 const& start, Vector3 const& end, float radius);
	~Capsule3() = default;

public:
	Vector3 m_start  = Vector3::Zero;
	Vector3 m_end    = Vector3::Zero;
	float   m_radius = 0.f;
};

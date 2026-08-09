#pragma once

#include "MingEngine/Core/Math/Vector3.hpp"

class Cylinder3
{
public:
	Cylinder3() = default;
	Cylinder3(Vector3 const& start, float height, float radius);
	~Cylinder3() = default;

public:
	Vector3 m_start;
	float   m_height;
	float   m_radius;
};
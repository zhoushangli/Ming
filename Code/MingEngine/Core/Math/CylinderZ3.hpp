#pragma once

#include "MingEngine/Core/Math/FloatRange.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

class CylinderZ3
{
public:
	CylinderZ3() = default;
	CylinderZ3(Vector3 const& start, float height, float radius);
	CylinderZ3(Vector2 const& centerXY, FloatRange const& minMaxZ, float radius);
	~CylinderZ3() = default;

	Vector3 const GetNearestPoint(Vector3 const& point) const;
	CylinderZ3    GetTransformed(Matrix4x4 const& transform) const;

public:
	Vector2    m_centerXY;
	float      m_radius;
	FloatRange m_minMaxZ;
};

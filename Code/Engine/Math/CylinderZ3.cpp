#include "Engine/Math/CylinderZ3.hpp"

#include "CylinderZ3.hpp"
#include "Engine/Math/MathUtils.hpp"

CylinderZ3::CylinderZ3(Vec3 const& start, float height, float radius)
	: m_centerXY(start.x, start.y), m_radius(radius), m_minMaxZ(start.z, start.z + height)
{
}

CylinderZ3::CylinderZ3(Vec2 const& centerXY, FloatRange const& minMaxZ, float radius)
	: m_centerXY(centerXY), m_radius(radius), m_minMaxZ(minMaxZ)
{
}

Vec3 const CylinderZ3::GetNearestPoint(Vec3 const& point) const { return GetNearestPointOnZCylinder3D(point, *this); }

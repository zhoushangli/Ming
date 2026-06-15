#include "MingEngine/Core/Math/CylinderZ3.hpp"

#include "CylinderZ3.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"

CylinderZ3::CylinderZ3(Vec3 const& start, float height, float radius)
	: m_centerXY(start.x, start.y), m_radius(radius), m_minMaxZ(start.z, start.z + height)
{
}

CylinderZ3::CylinderZ3(Vec2 const& centerXY, FloatRange const& minMaxZ, float radius)
	: m_centerXY(centerXY), m_radius(radius), m_minMaxZ(minMaxZ)
{
}

Vec3 const CylinderZ3::GetNearestPoint(Vec3 const& point) const { return GetNearestPointOnZCylinder3D(point, *this); }

CylinderZ3 CylinderZ3::GetTransformed(Matrix4x4 const& transform) const
{
	Vec2 worldCenter = m_centerXY + transform.GetTranslation2D();
	FloatRange worldMinMaxZ(m_minMaxZ.m_min + transform.GetTranslation3D().z, m_minMaxZ.m_max + transform.GetTranslation3D().z);

	return CylinderZ3(worldCenter, worldMinMaxZ, m_radius);
}

#include "Engine/Math/CylinderZ3.hpp"

#include "Engine/Math/MathUtils.hpp"

CylinderZ3::CylinderZ3(Vec3 const &start, float height, float radius)
    : m_start(start), m_height(height), m_radius(radius)
{
}

Vec3 const CylinderZ3::GetNearestPoint(Vec3 const &point) const
{
    return GetNearestPointOnZCylinder3D(point, *this);
}

FloatRange CylinderZ3::GetMinMaxZ() const
{
    return FloatRange(m_start.z, m_start.z + m_height);
}

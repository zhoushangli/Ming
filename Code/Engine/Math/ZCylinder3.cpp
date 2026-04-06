#include "Engine/Math/ZCylinder3.hpp"

#include "Engine/Math/MathUtils.hpp"

ZCylinder3::ZCylinder3(Vec3 const &start, float height, float radius)
    : m_start(start), m_height(height), m_radius(radius)
{
}

Vec3 const ZCylinder3::GetNearestPoint(Vec3 const &point) const
{
    return GetNearestPointOnZCylinder3D(point, *this);
}

FloatRange ZCylinder3::GetMinMaxZ() const
{
    return FloatRange(m_start.z, m_start.z + m_height);
}

#include "MingEngine/Core/Math/Sphere3.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"

Sphere3::Sphere3(Vec3 const &center, float radius)
    : m_center(center), m_radius(radius)
{
}

Vec3 const Sphere3::GetNearestPoint(Vec3 const &point) const
{
    return GetNearestPointOnSphere3D(point, *this);
}


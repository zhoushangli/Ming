#include "MingEngine/Core/Math/Sphere3.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"

using namespace Math;

Sphere3::Sphere3(Vector3 const& center, float radius) : m_center(center), m_radius(radius) {}

Vector3 const Sphere3::GetNearestPoint(Vector3 const& point) const { return GetNearestPointOnSphere3D(point, *this); }

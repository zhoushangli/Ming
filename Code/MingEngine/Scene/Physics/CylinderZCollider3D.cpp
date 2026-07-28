#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"

#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"

void CylinderZCollider3D::RenderDebug() const
{
	Vec2       worldStartXY = GetWorldTransform().TransformPosition2D(m_localStartXY);
	FloatRange worldZRange  = FloatRange(
		GetWorldTransform().TransformPosition3D(Vec3(0.f, 0.f, m_localZRange.m_min)).z,
		GetWorldTransform().TransformPosition3D(Vec3(0.f, 0.f, m_localZRange.m_max)).z);

	DebugGizmos::AddWorldWireCylinder(
		Vec3(worldStartXY.x, worldStartXY.y, worldZRange.m_min),
		Vec3(worldStartXY.x, worldStartXY.y, worldZRange.m_max),
		m_radius,
		0.f,
		Color::Cyan,
		Color::Cyan);
}

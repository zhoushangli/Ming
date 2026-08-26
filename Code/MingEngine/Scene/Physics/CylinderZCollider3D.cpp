#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"

#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"

void CylinderZCollider3D::RenderDebug() const
{
	Vector2    worldStartXY = GetWorldTransform().TransformPosition2D(m_localStartXY);
	FloatRange worldZRange  = FloatRange(
		GetWorldTransform().TransformPosition3D(Vector3(0.f, 0.f, m_localZRange.m_min)).z,
		GetWorldTransform().TransformPosition3D(Vector3(0.f, 0.f, m_localZRange.m_max)).z);

	DebugGizmos::AddWorldWireCylinder(
		Vector3(worldStartXY.x, worldStartXY.y, worldZRange.m_min),
		Vector3(worldStartXY.x, worldStartXY.y, worldZRange.m_max),
		m_radius,
		0.f,
		Color::Cyan,
		Color::Cyan);
}

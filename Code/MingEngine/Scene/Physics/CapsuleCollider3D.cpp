#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"

#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"

CapsuleCollider3D::CapsuleCollider3D(Capsule3 const& capsule)
{
	m_localStart = capsule.m_start;
	m_localEnd   = capsule.m_end;
	m_radius     = capsule.m_radius;
}

void CapsuleCollider3D::RenderDebug() const
{
	Vector3 worldStart = GetWorldTransform().TransformPosition3D(m_localStart);
	Vector3 worldEnd   = GetWorldTransform().TransformPosition3D(m_localEnd);

	DebugGizmos::AddWorldWireCapsule(worldStart, worldEnd, m_radius, 0.f, Color::Orange, Color::Orange);
}

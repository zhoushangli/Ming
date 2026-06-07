#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"

#include "MingEngine/Engine/Render/Rgba8.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/RaycastUtils.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"

CapsuleCollider3D::CapsuleCollider3D(Capsule3 const& capsule)
{
	m_localStart = capsule.m_start;
	m_localEnd   = capsule.m_end;
	m_radius     = capsule.m_radius;
}

GameRaycastResult CapsuleCollider3D::Raycast(RaycastInfo const& info) const
{
	GameRaycastResult raycastResult;

	Matrix4x4 worldTransform = GetWorldTransform();
	Capsule3  capsule(
		worldTransform.TransformPosition3D(m_localStart),
		worldTransform.TransformPosition3D(m_localEnd),
		m_radius
	);
	(RaycastResult3D&)raycastResult =
		RaycastVsCapsule3D(info.m_startPos, info.m_forwardNormal, info.m_maxLength, capsule);

	if (raycastResult.m_didImpact)
	{
		raycastResult.m_hitNodeHandle = GetHandle();
	}

	return raycastResult;
}

void CapsuleCollider3D::Update(float deltaSeconds) { Node3D::Update(deltaSeconds); }

void CapsuleCollider3D::RenderDebug() const
{
	Vec3 worldStart = GetWorldTransform().TransformPosition3D(m_localStart);
	Vec3 worldEnd   = GetWorldTransform().TransformPosition3D(m_localEnd);

	DebugAddWorldWireCapsule(worldStart, worldEnd, m_radius, 0.f, Rgba8::Orange, Rgba8::Orange);
}

#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"

#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/RaycastUtils.hpp"

AABBCollider3D::AABBCollider3D(AABB3 const& localBounds)
	: m_localBounds(localBounds)
{
}

GameRaycastResult AABBCollider3D::Raycast(RaycastInfo const& info) const
{
	GameRaycastResult raycastResult;
	Matrix4x4         localToWorld = GetWorldTransform();
	Matrix4x4 const   worldToLocal = localToWorld.GetOrthonormalInverse();
	Vec3 const        localStart   = worldToLocal.TransformPosition3D(info.m_startPos);
	Vec3 const        localForward = worldToLocal.TransformDirection3D(info.m_forwardNormal);

	(RaycastResult3D&)raycastResult = RaycastVsAABB3D(localStart, localForward, info.m_maxLength, m_localBounds);

	if (raycastResult.m_didImpact)
	{
		raycastResult.m_impactPos     = localToWorld.TransformPosition3D(raycastResult.m_impactPos);
		raycastResult.m_impactNormal  = localToWorld.TransformDirection3D(raycastResult.m_impactNormal);
		raycastResult.m_hitNodeHandle = GetHandle();
	}

	raycastResult.m_rayStartPos  = info.m_startPos;
	raycastResult.m_rayFwdNormal = info.m_forwardNormal;
	raycastResult.m_rayMaxLength = info.m_maxLength;

	return raycastResult;
}

void AABBCollider3D::Update(float deltaSeconds) { Node3D::Update(deltaSeconds); }

void AABBCollider3D::RenderDebug() const
{
}

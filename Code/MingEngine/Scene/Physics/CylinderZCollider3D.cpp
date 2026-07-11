#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"

#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"

GameRaycastResult CylinderZCollider3D::Raycast(RaycastInfo const& info) const
{
	GameRaycastResult raycastResult;

	Matrix4x4  worldTransform = GetWorldTransform();
	Vec2       worldStartXY   = worldTransform.TransformPosition2D(m_localStartXY);
	FloatRange worldZRange    = FloatRange(
		worldTransform.TransformPosition3D(Vec3(0.f, 0.f, m_localZRange.m_min)).z,
		worldTransform.TransformPosition3D(Vec3(0.f, 0.f, m_localZRange.m_max)).z);

	(MathRaycastResult3D&)raycastResult = RaycastVsCylinderZ3D(
		info.m_startPos,
		info.m_forwardNormal,
		info.m_maxLength,
		worldStartXY,
		worldZRange,
		m_radius);

	if (raycastResult.m_didImpact)
	{
		raycastResult.m_hitNodeHandle = GetHandle();
	}

	return raycastResult;
}

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
		Rgba8::Cyan,
		Rgba8::Cyan);
}

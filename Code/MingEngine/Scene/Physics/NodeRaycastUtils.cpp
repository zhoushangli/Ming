#include "MingEngine/Scene/Physics/NodeRaycastUtils.hpp"

#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/Vec4.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"

#include <cmath>

namespace
{
Vec3 HomogeneousToPosition(Vec4 const& homogeneous)
{
	if (fabsf(homogeneous.w) <= 1e-6f)
	{
		return Vec3(homogeneous.x, homogeneous.y, homogeneous.z);
	}

	float const invW = 1.f / homogeneous.w;
	return Vec3(homogeneous.x * invW, homogeneous.y * invW, homogeneous.z * invW);
}
} // namespace

RaycastInfo BuildRaycastFromMouse(
	CameraContext const& camera, Vec2 const& clientPos, Vec2 const& clientDimensions, float maxLength)
{
	RaycastInfo info;
	info.m_maxLength = maxLength;

	if (clientDimensions.x <= 0.f || clientDimensions.y <= 0.f)
	{
		info.m_startPos      = camera.GetPosition();
		info.m_forwardNormal = camera.GetOrientation().GetForwardDir_IFwd_JLeft_KUp();
		return info;
	}

	float const clipX = 2.f * (clientPos.x / clientDimensions.x) - 1.f;
	float const clipY = 1.f - 2.f * (clientPos.y / clientDimensions.y);

	Matrix4x4 const clipToCamera  = camera.GetClipToCameraTransform();
	Matrix4x4 const cameraToWorld = camera.GetCameraToWorldTransform();

	Vec3 const nearCameraPos = HomogeneousToPosition(clipToCamera.TransformHomogeneous3D(Vec4(clipX, clipY, 0.f, 1.f)));
	Vec3 const farCameraPos  = HomogeneousToPosition(clipToCamera.TransformHomogeneous3D(Vec4(clipX, clipY, 1.f, 1.f)));
	Vec3 const nearWorldPos  = cameraToWorld.TransformPosition3D(nearCameraPos);
	Vec3 const farWorldPos   = cameraToWorld.TransformPosition3D(farCameraPos);

	if (camera.GetMode() == CameraContext::Perspective)
	{
		info.m_startPos      = camera.GetPosition();
		info.m_forwardNormal = (farWorldPos - info.m_startPos).GetNormalized();
	}
	else
	{
		info.m_startPos      = nearWorldPos;
		info.m_forwardNormal = (farWorldPos - nearWorldPos).GetNormalized();
	}

	if (info.m_forwardNormal.GetLengthSquared() <= 1e-6f)
	{
		info.m_forwardNormal = camera.GetOrientation().GetForwardDir_IFwd_JLeft_KUp();
	}

	return info;
}

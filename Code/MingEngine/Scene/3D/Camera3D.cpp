#include "MingEngine/Scene/3D/Camera3D.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

namespace
{
// clang-format off
Matrix4x4 const CameraToRenderTransform_Perspective = Matrix4x4(
	0.f, -1.f, 0.f, 0.f, 
	0.f, 0.f, 1.f, 0.f, 
	1.f, 0.f, 0.f, 0.f, 
	0.f, 0.f, 0.f, 1.f);
// clang-format on
} // namespace

Camera3D::Camera3D(float fovDegrees, float nearClip, float farClip)
	: m_mode(CameraContext::Perspective), m_nearClip(nearClip), m_farClip(farClip), m_fovDegrees(fovDegrees),
	  m_size(1.f)
{
}

void Camera3D::BindMethods()
{
	ClassDatabase::BindMethod("SetNearClip", &Camera3D::SetNearClip);
	ClassDatabase::BindMethod("GetNearClip", &Camera3D::GetNearClip);
	ClassDatabase::BindMethod("SetFarClip", &Camera3D::SetFarClip);
	ClassDatabase::BindMethod("GetFarClip", &Camera3D::GetFarClip);
	ClassDatabase::BindMethod("SetFovDegrees", &Camera3D::SetFovDegrees);
	ClassDatabase::BindMethod("GetFovDegrees", &Camera3D::GetFovDegrees);
	ClassDatabase::BindMethod("SetSize", &Camera3D::SetSize);
	ClassDatabase::BindMethod("GetSize", &Camera3D::GetSize);

	PropertyInfo::UsageFlags const usage = PropertyInfo::UsageFlags::Default;
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "near_clip", PropertyInfo::Hint::None, "", usage),
		"SetNearClip",
		"GetNearClip");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "far_clip", PropertyInfo::Hint::None, "", usage),
		"SetFarClip",
		"GetFarClip");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "fov_degrees", PropertyInfo::Hint::None, "", usage),
		"SetFovDegrees",
		"GetFovDegrees");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Float, "size", PropertyInfo::Hint::None, "", usage), "SetSize", "GetSize");
}

void Camera3D::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
		m_data.m_viewport->RegisterWorldCamera(this);
		break;
	case NotificationType::ExitTree:
		m_data.m_viewport->UnregisterWorldCamera(this);
		break;
	}
}

CameraContext Camera3D::GetCameraContext(float aspect) const
{
	CameraContext camera;
	camera.SetTransform(GetWorldTransform());

	if (m_mode == CameraContext::Orthographic)
	{
		camera.SetCameraToRenderTransform(Matrix4x4::Identity);
		camera.SetOrthogonal(Vec2::Zero, Vec2(m_size * aspect, m_size), m_nearClip, m_farClip);
	}
	else if (m_mode == CameraContext::Perspective)
	{
		camera.SetCameraToRenderTransform(CameraToRenderTransform_Perspective);
		camera.SetPerspective(aspect, m_fovDegrees, m_nearClip, m_farClip);
	}
	return camera;
}

MathRaycastQuery3D Camera3D::BuildRaycastFromMouse(
	Vec2 const& mousePos, Vec2 const& viewportDimensions, float maxLength) const
{
	GUARANTEE_OR_DIE(m_mode == CameraContext::Perspective, "Camera3D mouse raycast only supports perspective cameras");

	MathRaycastQuery3D raycastInfo;
	raycastInfo.m_startPos      = GetWorldPosition();
	raycastInfo.m_forwardNormal = GetWorldForward();
	raycastInfo.m_maxLength     = maxLength;

	if (viewportDimensions.x <= 0.f || viewportDimensions.y <= 0.f)
	{
		return raycastInfo;
	}

	float const screenX       = 2.f * (mousePos.x / viewportDimensions.x) - 1.f;
	float const screenY       = 1.f - 2.f * (mousePos.y / viewportDimensions.y);
	float const aspect        = viewportDimensions.x / viewportDimensions.y;
	float const halfFovDegrees = m_fovDegrees * 0.5f;
	float const halfHeight     = Math::SinDegrees(halfFovDegrees) / Math::CosDegrees(halfFovDegrees);
	float const halfWidth      = halfHeight * aspect;

	Vec3 forward;
	Vec3 left;
	Vec3 up;
	GetWorldOrientation().GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

	raycastInfo.m_forwardNormal =
		(forward - left * screenX * halfWidth + up * screenY * halfHeight).GetNormalized();
	return raycastInfo;
}

void Camera3D::SetOrthogonal(float size, float nearClip, float farClip)
{
	m_mode     = CameraContext::Orthographic;
	m_size     = size;
	m_nearClip = nearClip;
	m_farClip  = farClip;
}

void Camera3D::SetPerspective(float fovDegrees, float nearClip, float farClip)
{
	m_mode       = CameraContext::Perspective;
	m_fovDegrees = fovDegrees;
	m_nearClip   = nearClip;
	m_farClip    = farClip;
}

CameraContext::Mode Camera3D::GetMode() const { return m_mode; }

void Camera3D::SetMode(CameraContext::Mode mode) { m_mode = mode; }

float Camera3D::GetNearClip() const { return m_nearClip; }

void Camera3D::SetNearClip(float nearClip) { m_nearClip = nearClip; }

float Camera3D::GetFarClip() const { return m_farClip; }

void Camera3D::SetFarClip(float farClip) { m_farClip = farClip; }

float Camera3D::GetFovDegrees() const { return m_fovDegrees; }

void Camera3D::SetFovDegrees(float fovDegrees) { m_fovDegrees = fovDegrees; }

float Camera3D::GetSize() const { return m_size; }

void Camera3D::SetSize(float size) { m_size = size; }

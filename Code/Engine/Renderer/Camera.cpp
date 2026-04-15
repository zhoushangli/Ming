#include "Engine/Renderer/Camera.hpp"

#include "Camera.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

void Camera::
	SetOrthographicView(Vec2 const& bottomLeft, Vec2 const& topRight, float near /*= 0.0f*/, float far /*= 1.0f*/)
{
	m_mode = eMode_Orthographic;

	m_orthographicBottomLeft = bottomLeft;
	m_orthographicTopRight   = topRight;
	m_orthographicNear       = near;
	m_orthographicFar        = far;
}

void Camera::SetPerspectiveView(float aspect, float fov, float near, float far)
{
	m_mode = eMode_Perspective;

	m_perspectiveAspect = aspect;
	m_perspectiveFOV    = fov;
	m_perspectiveNear   = near;
	m_perspectiveFar    = far;
}

void Camera::SetPositionAndOrientation(const Vec3& position, const EulerAngles& orientation)
{
	m_position    = position;
	m_orientation = orientation;
}

void Camera::SetPosition(const Vec3& position) { m_position = position; }

Vec3 Camera::GetPosition() const { return m_position; }

void Camera::SetOrientation(const EulerAngles& orientation) { m_orientation = orientation; }

EulerAngles Camera::GetOrientation() const { return m_orientation; }

Matrix4x4 Camera::GetCameraToWorldTransform() const
{
	Matrix4x4 camToWorld;
	camToWorld.SetTranslation3D(m_position);
	camToWorld.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return camToWorld;
}

Matrix4x4 Camera::GetWorldToCameraTransform() const { return GetCameraToWorldTransform().GetOrthonormalInverse(); }

AABB2 Camera::GetOrthographicBounds() const { return AABB2(m_orthographicBottomLeft, m_orthographicTopRight); }

void Camera::SetCameraToRenderTransform(const Matrix4x4& m) { m_cameraToRenderTransform = m; }

Matrix4x4 Camera::GetCameraToRenderTransform() const { return m_cameraToRenderTransform; }

Matrix4x4 Camera::GetRenderToClipTransform() const { return GetProjectionMatrix(); }

Vec2 Camera::GetOrthographicBottomLeft() const { return m_orthographicBottomLeft; }

Vec2 Camera::GetOrthographicTopRight() const { return m_orthographicTopRight; }

void Camera::Translate2D(Vec2 const& translation)
{
	m_position.x += translation.x;
	m_position.y += translation.y;

	m_orthographicBottomLeft += translation;
	m_orthographicTopRight += translation;
}

Matrix4x4 Camera::GetOrthographicMatrix() const
{
	return Matrix4x4::MakeOrthoProjection(
		m_orthographicBottomLeft.x,
		m_orthographicTopRight.x,
		m_orthographicBottomLeft.y,
		m_orthographicTopRight.y,
		m_orthographicNear,
		m_orthographicFar
	);
}

Matrix4x4 Camera::GetPerspectiveMatrix() const
{
	return Matrix4x4::MakePerspectiveProjection(
		m_perspectiveFOV,
		m_perspectiveAspect,
		m_perspectiveNear,
		m_perspectiveFar
	);
}

Matrix4x4 Camera::GetProjectionMatrix() const
{
	return (m_mode == eMode_Perspective) ? GetPerspectiveMatrix() : GetOrthographicMatrix();
}

float Camera::GetNearZ() const
{
	if (m_mode == eMode_Perspective)
		return m_perspectiveNear;
	else
		return m_orthographicNear;
}

float Camera::GetFarZ() const
{
	if (m_mode == eMode_Perspective)
		return m_perspectiveFar;
	else
		return m_orthographicFar;
}
#include "Engine/Renderer/Camera.hpp"

#include "Camera.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Math/MathUtils.hpp"
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

Camera::Mode Camera::GetMode() const { return m_mode; }

void Camera::SetCameraToRenderTransform(const Matrix4x4& m) { m_cameraToRenderTransform = m; }

Matrix4x4 Camera::GetCameraToRenderTransform() const { return m_cameraToRenderTransform; }

Matrix4x4 Camera::GetRenderToClipTransform() const { return GetProjectionMatrix(); }

Matrix4x4 Camera::GetClipToCameraTransform() const
{
	Matrix4x4 clipToRender   = GetProjectionInverseMatrix();
	Matrix4x4 renderToCamera = GetCameraToRenderTransform().GetOrthonormalInverse();
	renderToCamera.Append(clipToRender);
	return renderToCamera;
}

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

Matrix4x4 Camera::GetOrthographicInverseMatrix() const
{
	float l = m_orthographicBottomLeft.x;
	float r = m_orthographicTopRight.x;
	float b = m_orthographicBottomLeft.y;
	float t = m_orthographicTopRight.y;
	float n = m_orthographicNear;
	float f = m_orthographicFar;

	Matrix4x4 inverse = Matrix4x4::ZERO;
	inverse.m_values[Matrix4x4::Ix] = (r - l) * 0.5f;
	inverse.m_values[Matrix4x4::Jy] = (t - b) * 0.5f;
	inverse.m_values[Matrix4x4::Kz] = (f - n);
	inverse.m_values[Matrix4x4::Tx] = (r + l) * 0.5f;
	inverse.m_values[Matrix4x4::Ty] = (t + b) * 0.5f;
	inverse.m_values[Matrix4x4::Tz] = n;
	inverse.m_values[Matrix4x4::Tw] = 1.0f;
	return inverse;
}

Matrix4x4 Camera::GetPerspectiveInverseMatrix() const
{
	float c      = CosDegrees(m_perspectiveFOV * 0.5f);
	float s      = SinDegrees(m_perspectiveFOV * 0.5f);
	float scaleY = c / s;
	float scaleX = scaleY / m_perspectiveAspect;

	float zNear = m_perspectiveNear;
	float zFar  = m_perspectiveFar;
	float scaleZ = zFar / (zFar - zNear);
	float translateZ = (zNear * zFar) / (zNear - zFar);

	Matrix4x4 inverse = Matrix4x4::ZERO;
	inverse.m_values[Matrix4x4::Ix] = 1.0f / scaleX;
	inverse.m_values[Matrix4x4::Jy] = 1.0f / scaleY;
	inverse.m_values[Matrix4x4::Tz] = 1.0f;
	inverse.m_values[Matrix4x4::Kw] = 1.0f / translateZ;
	inverse.m_values[Matrix4x4::Tw] = -scaleZ / translateZ;
	return inverse;
}

Matrix4x4 Camera::GetProjectionInverseMatrix() const
{
	return (m_mode == eMode_Perspective) ? GetPerspectiveInverseMatrix() : GetOrthographicInverseMatrix();
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

#include "MingEngine/Engine/Render/CameraContext.hpp"

#include "CameraContext.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

using namespace Math;

void CameraContext::SetOrthogonal(
	Vector2 const& bottomLeft, Vector2 const& topRight, float near /*= 0.0f*/, float far /*= 1.0f*/)
{
	m_mode = Orthographic;

	m_orthographicBottomLeft = bottomLeft;
	m_orthographicTopRight   = topRight;
	m_orthographicNear       = near;
	m_orthographicFar        = far;
}

void CameraContext::SetPerspective(float aspect, float fov, float near, float far)
{
	m_mode = Perspective;

	m_perspectiveAspect = aspect;
	m_perspectiveFOV    = fov;
	m_perspectiveNear   = near;
	m_perspectiveFar    = far;
}

void CameraContext::SetTransform(const Matrix4x4& cameraToWorld)
{
	m_position = cameraToWorld.GetTranslation3D();
	m_orientation.SetFromMatrix_IFwd_JLeft_KUp(cameraToWorld);
}

void CameraContext::SetPositionAndOrientation(const Vector3& position, const EulerAngles& orientation)
{
	m_position    = position;
	m_orientation = orientation;
}

void CameraContext::SetPosition(const Vector3& position) { m_position = position; }

Vector3 CameraContext::GetPosition() const { return m_position; }

void CameraContext::SetOrientation(const EulerAngles& orientation) { m_orientation = orientation; }

EulerAngles CameraContext::GetOrientation() const { return m_orientation; }

Matrix4x4 CameraContext::GetCameraToWorldTransform() const
{
	Matrix4x4 camToWorld;
	camToWorld.SetTranslation3D(m_position);
	camToWorld.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return camToWorld;
}

Matrix4x4 CameraContext::GetWorldToCameraTransform() const
{
	return GetCameraToWorldTransform().GetOrthonormalInverse();
}

AABB2 CameraContext::GetOrthographicBounds() const { return AABB2(m_orthographicBottomLeft, m_orthographicTopRight); }

CameraContext::Mode CameraContext::GetMode() const { return m_mode; }

void CameraContext::SetCameraToRenderTransform(const Matrix4x4& m) { m_cameraToRenderTransform = m; }

Matrix4x4 CameraContext::GetCameraToRenderTransform() const { return m_cameraToRenderTransform; }

Matrix4x4 CameraContext::GetRenderToClipTransform() const { return GetProjectionMatrix(); }

Matrix4x4 CameraContext::GetClipToCameraTransform() const
{
	Matrix4x4 clipToRender   = GetProjectionInverseMatrix();
	Matrix4x4 renderToCamera = GetCameraToRenderTransform().GetOrthonormalInverse();
	renderToCamera.Append(clipToRender);
	return renderToCamera;
}

Vector2 CameraContext::GetOrthographicBottomLeft() const { return m_orthographicBottomLeft; }

Vector2 CameraContext::GetOrthographicTopRight() const { return m_orthographicTopRight; }

void CameraContext::Translate2D(Vector2 const& translation)
{
	m_position.x += translation.x;
	m_position.y += translation.y;

	m_orthographicBottomLeft += translation;
	m_orthographicTopRight += translation;
}

Matrix4x4 CameraContext::GetOrthographicMatrix() const
{
	return Matrix4x4::MakeOrthoProjection(
		m_orthographicBottomLeft.x,
		m_orthographicTopRight.x,
		m_orthographicBottomLeft.y,
		m_orthographicTopRight.y,
		m_orthographicNear,
		m_orthographicFar);
}

Matrix4x4 CameraContext::GetPerspectiveMatrix() const
{
	return Matrix4x4::MakePerspectiveProjection(
		m_perspectiveFOV,
		m_perspectiveAspect,
		m_perspectiveNear,
		m_perspectiveFar);
}

Matrix4x4 CameraContext::GetProjectionMatrix() const
{
	return (m_mode == Perspective) ? GetPerspectiveMatrix() : GetOrthographicMatrix();
}

Matrix4x4 CameraContext::GetOrthographicInverseMatrix() const
{
	float l = m_orthographicBottomLeft.x;
	float r = m_orthographicTopRight.x;
	float b = m_orthographicBottomLeft.y;
	float t = m_orthographicTopRight.y;
	float n = m_orthographicNear;
	float f = m_orthographicFar;

	Matrix4x4 inverse               = Matrix4x4::Zero;
	inverse.m_values[Matrix4x4::Ix] = (r - l) * 0.5f;
	inverse.m_values[Matrix4x4::Jy] = (t - b) * 0.5f;
	inverse.m_values[Matrix4x4::Kz] = (f - n);
	inverse.m_values[Matrix4x4::Tx] = (r + l) * 0.5f;
	inverse.m_values[Matrix4x4::Ty] = (t + b) * 0.5f;
	inverse.m_values[Matrix4x4::Tz] = n;
	inverse.m_values[Matrix4x4::Tw] = 1.0f;
	return inverse;
}

Matrix4x4 CameraContext::GetPerspectiveInverseMatrix() const
{
	float c      = CosDegrees(m_perspectiveFOV * 0.5f);
	float s      = SinDegrees(m_perspectiveFOV * 0.5f);
	float scaleY = c / s;
	float scaleX = scaleY / m_perspectiveAspect;

	float zNear      = m_perspectiveNear;
	float zFar       = m_perspectiveFar;
	float scaleZ     = zFar / (zFar - zNear);
	float translateZ = (zNear * zFar) / (zNear - zFar);

	Matrix4x4 inverse               = Matrix4x4::Zero;
	inverse.m_values[Matrix4x4::Ix] = 1.0f / scaleX;
	inverse.m_values[Matrix4x4::Jy] = 1.0f / scaleY;
	inverse.m_values[Matrix4x4::Tz] = 1.0f;
	inverse.m_values[Matrix4x4::Kw] = 1.0f / translateZ;
	inverse.m_values[Matrix4x4::Tw] = -scaleZ / translateZ;
	return inverse;
}

Matrix4x4 CameraContext::GetProjectionInverseMatrix() const
{
	return (m_mode == Perspective) ? GetPerspectiveInverseMatrix() : GetOrthographicInverseMatrix();
}

float CameraContext::GetNearZ() const
{
	if (m_mode == Perspective)
		return m_perspectiveNear;
	else
		return m_orthographicNear;
}

float CameraContext::GetFarZ() const
{
	if (m_mode == Perspective)
		return m_perspectiveFar;
	else
		return m_orthographicFar;
}

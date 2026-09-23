#include "MingEngine/Engine/Render/Projection.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"

using namespace Math;

namespace
{

// clang-format off
Matrix4x4 const kCameraToRenderTransform_Perspective = Matrix4x4(
	0.f, -1.f, 0.f, 0.f, 
	0.f, 0.f, 1.f, 0.f, 
	1.f, 0.f, 0.f, 0.f, 
	0.f, 0.f, 0.f, 1.f);
// clang-format on

// Orthographic cameras cover (0,0) to (size * aspect, size).
// e.g. size 10 with aspect 2 covers x in [0, 20] and y in [0, 10]
Matrix4x4 MakeOrthographicProjection(CameraData const& camera, float aspect)
{
	return Matrix4x4::MakeOrthoProjection(
		0.f,
		camera.m_size * aspect,
		0.f,
		camera.m_size,
		camera.m_nearZ,
		camera.m_farZ);
}

Matrix4x4 MakeOrthographicInverseProjection(CameraData const& camera, float aspect)
{
	float l = 0.f;
	float r = camera.m_size * aspect;
	float b = 0.f;
	float t = camera.m_size;
	float n = camera.m_nearZ;
	float f = camera.m_farZ;

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

Matrix4x4 MakePerspectiveProjection(CameraData const& camera, float aspect)
{
	return Matrix4x4::MakePerspectiveProjection(camera.m_fovDegrees, aspect, camera.m_nearZ, camera.m_farZ);
}

Matrix4x4 MakePerspectiveInverseProjection(CameraData const& camera, float aspect)
{
	float c      = CosDegrees(camera.m_fovDegrees * 0.5f);
	float s      = SinDegrees(camera.m_fovDegrees * 0.5f);
	float scaleY = c / s;
	float scaleX = scaleY / aspect;

	float zNear      = camera.m_nearZ;
	float zFar       = camera.m_farZ;
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

} // namespace

// Every matrix is derived once so camera binding and post process share one result.
// e.g. a Perspective camera gets kCameraToRenderTransform_Perspective, an Orthographic one gets Identity
Projection::Projection(CameraData const& camera, float aspect) : m_nearZ(camera.m_nearZ), m_farZ(camera.m_farZ)
{
	m_cameraToWorld = camera.m_cameraToWorld;
	m_worldToCamera = m_cameraToWorld.GetOrthonormalInverse();

	bool const isPerspective = camera.m_mode == CameraMode::Perspective;

	m_cameraToRender = isPerspective ? kCameraToRenderTransform_Perspective : Matrix4x4::Identity;
	m_renderToClip =
		isPerspective ? MakePerspectiveProjection(camera, aspect) : MakeOrthographicProjection(camera, aspect);

	Matrix4x4 renderToClipInverse = isPerspective ? MakePerspectiveInverseProjection(camera, aspect)
												  : MakeOrthographicInverseProjection(camera, aspect);

	// clipToCamera reverses the camera-to-render transform first, then the projection
	Matrix4x4 renderToCamera = m_cameraToRender.GetOrthonormalInverse();
	renderToCamera.Append(renderToClipInverse);
	m_clipToCamera = renderToCamera;
}

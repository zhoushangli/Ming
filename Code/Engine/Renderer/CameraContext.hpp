#pragma once

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec2.hpp"

class CameraContext
{
public:
	enum Mode
	{
		Orthographic,
		Perspective,
		Count
	};

	void SetOrthogonal(Vec2 const& bottomLeft, Vec2 const& topRight, float near = 0.0f, float far = 1.0f);
	void SetPerspective(float aspect, float fov, float near, float far);

	void        SetTransform(const Matrix4x4& cameraToWorld);
	void        SetPositionAndOrientation(const Vec3& position, const EulerAngles& orientation);
	void        SetPosition(const Vec3& position);
	Vec3        GetPosition() const;
	void        SetOrientation(const EulerAngles& orientation);
	EulerAngles GetOrientation() const;

	Matrix4x4 GetCameraToWorldTransform() const;
	Matrix4x4 GetWorldToCameraTransform() const;
	AABB2     GetOrthographicBounds() const;
	Mode      GetMode() const;

	void      SetCameraToRenderTransform(const Matrix4x4& m);
	Matrix4x4 GetCameraToRenderTransform() const;
	Matrix4x4 GetRenderToClipTransform() const;
	Matrix4x4 GetClipToCameraTransform() const;

	Vec2 GetOrthographicBottomLeft() const;
	Vec2 GetOrthographicTopRight() const;
	void Translate2D(Vec2 const& translation);

	Matrix4x4 GetOrthographicMatrix() const;
	Matrix4x4 GetPerspectiveMatrix() const;
	Matrix4x4 GetProjectionMatrix() const;
	Matrix4x4 GetOrthographicInverseMatrix() const;
	Matrix4x4 GetPerspectiveInverseMatrix() const;
	Matrix4x4 GetProjectionInverseMatrix() const;

	float GetNearZ() const;
	float GetFarZ() const;

protected:
	Mode m_mode = Orthographic;

	Vec3        m_position    = Vec3::Zero;
	EulerAngles m_orientation = EulerAngles::Zero;

	Vec2  m_orthographicBottomLeft = Vec2::Zero;
	Vec2  m_orthographicTopRight   = Vec2::Zero;
	float m_orthographicNear       = 0.0f;
	float m_orthographicFar        = 1.0f;

	float m_perspectiveAspect = 1.0f;
	float m_perspectiveFOV    = 0.0f;
	float m_perspectiveNear   = 0.0f;
	float m_perspectiveFar    = 1.0f;

	Matrix4x4 m_cameraToRenderTransform = Matrix4x4::Identity;
};

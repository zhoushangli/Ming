#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"

class CameraContext
{
public:
	enum Mode
	{
		Orthographic,
		Perspective,
		Count
	};

	void SetOrthogonal(Vector2 const& bottomLeft, Vector2 const& topRight, float near = 0.0f, float far = 1.0f);
	void SetPerspective(float aspect, float fov, float near, float far);

	void        SetTransform(const Matrix4x4& cameraToWorld);
	void        SetPositionAndOrientation(const Vector3& position, const EulerAngles& orientation);
	void        SetPosition(const Vector3& position);
	Vector3     GetPosition() const;
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

	Vector2 GetOrthographicBottomLeft() const;
	Vector2 GetOrthographicTopRight() const;
	void    Translate2D(Vector2 const& translation);

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

	Vector3     m_position    = Vector3::Zero;
	EulerAngles m_orientation = EulerAngles::Zero;

	Vector2 m_orthographicBottomLeft = Vector2::Zero;
	Vector2 m_orthographicTopRight   = Vector2::Zero;
	float   m_orthographicNear       = 0.0f;
	float   m_orthographicFar        = 1.0f;

	float m_perspectiveAspect = 1.0f;
	float m_perspectiveFOV    = 0.0f;
	float m_perspectiveNear   = 0.0f;
	float m_perspectiveFar    = 1.0f;

	Matrix4x4 m_cameraToRenderTransform = Matrix4x4::Identity;
};

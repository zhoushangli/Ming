#pragma once

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec2.hpp"

class Camera
{
public:
	enum Mode
	{
		eMode_Orthographic,
		eMode_Perspective,

		eMode_Count
	};

	void SetOrthographicView(Vec2 const& bottomLeft, Vec2 const& topRight, float near = 0.0f, float far = 1.0f);
	void SetPerspectiveView(float aspect, float fov, float near, float far);

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

	Vec2  GetOrthographicBottomLeft() const;
	Vec2  GetOrthographicTopRight() const;
	void  Translate2D(Vec2 const& translation);
	void  SetViewportNormalized(AABB2 const& viewportNormalized);
	AABB2 GetViewportNormalized() const;

	Matrix4x4 GetOrthographicMatrix() const;
	Matrix4x4 GetPerspectiveMatrix() const;
	Matrix4x4 GetProjectionMatrix() const;
	Matrix4x4 GetOrthographicInverseMatrix() const;
	Matrix4x4 GetPerspectiveInverseMatrix() const;
	Matrix4x4 GetProjectionInverseMatrix() const;

	float GetNearZ() const;
	float GetFarZ() const;

protected:
	Mode m_mode = eMode_Orthographic;

	Vec3        m_position;
	EulerAngles m_orientation;

	Vec2  m_orthographicBottomLeft;
	Vec2  m_orthographicTopRight;
	float m_orthographicNear;
	float m_orthographicFar;

	float m_perspectiveAspect;
	float m_perspectiveFOV;
	float m_perspectiveNear;
	float m_perspectiveFar;
	AABB2 m_viewportNormalized = AABB2(Vec2(0.f, 0.f), Vec2(1.f, 1.f));

	Matrix4x4 m_cameraToRenderTransform;
};

#pragma once

#include "MingEngine/Core/Math/Matrix4x4.hpp"

struct CameraData;

// Prepared view matrices for one CameraData and one viewport aspect.
// e.g. Projection projection(*cameraData, aspect); backend.BindCamera(projection)
class Projection
{
public:
	Projection(CameraData const& camera, float aspect);

	Matrix4x4 GetCameraToWorldTransform() const { return m_cameraToWorld; }
	Matrix4x4 GetWorldToCameraTransform() const { return m_worldToCamera; }
	Matrix4x4 GetCameraToRenderTransform() const { return m_cameraToRender; }
	Matrix4x4 GetRenderToClipTransform() const { return m_renderToClip; }
	Matrix4x4 GetClipToCameraTransform() const { return m_clipToCamera; }

	float GetNearZ() const { return m_nearZ; }
	float GetFarZ() const { return m_farZ; }

private:
	Matrix4x4 m_cameraToWorld  = Matrix4x4::Identity;
	Matrix4x4 m_worldToCamera  = Matrix4x4::Identity;
	Matrix4x4 m_cameraToRender = Matrix4x4::Identity;
	Matrix4x4 m_renderToClip   = Matrix4x4::Identity;
	Matrix4x4 m_clipToCamera   = Matrix4x4::Identity;

	float m_nearZ = 0.f;
	float m_farZ  = 1.f;
};

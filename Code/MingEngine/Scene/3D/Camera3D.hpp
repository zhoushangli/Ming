#pragma once

#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Render/CameraContext.hpp"

class Camera3D : public Node3D
{
	CLASS(Camera3D, Node3D);

public:
	Camera3D(float fovDegrees = 60.f, float nearClip = 0.1f, float farClip = 100.f);
	~Camera3D() override = default;

	// The aspect will be determined by the viewport so it is not a parameter of Camera3D
	// Instead, it will be passed in when GetCamera is called.
	CameraContext GetCamera(float aspect) const;

	void SetOrthogonal(float size, float nearClip = 0.f, float farClip = 1.f);
	void SetPerspective(float fovDegrees, float nearClip = 0.1f, float farClip = 100.f);

	CameraContext::Mode GetMode() const;
	float        GetNearClip() const;
	float        GetFarClip() const;
	float        GetFovDegrees() const;
	float        GetSize() const;

	void SetMode(CameraContext::Mode mode);
	void SetNearClip(float nearClip);
	void SetFarClip(float farClip);
	void SetFovDegrees(float fovDegrees);
	void SetSize(float size);

private:
	CameraContext::Mode m_mode;
	float        m_nearClip;
	float        m_farClip;

	// Perspective parameters:
	float m_fovDegrees;

	// Orthographic parameters:
	float m_size; // the size of y axis
};

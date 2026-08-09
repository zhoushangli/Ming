#pragma once

#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Render/CameraContext.hpp"

// Camera node that defers aspect ratio to the caller.
// Aspect is a viewport property, not a camera property — it is supplied
// via GetCameraContext(aspect) at render time.

// Both m_fovDegrees and m_size are measured on the vertical (Y) axis;
// the horizontal axis is derived from Y / aspect
// we do this because when we have different screen
// we want 16 : 9, 4 : 3, ... looks the same in height
class Camera3D : public Node3D
{
	MCLASS(Camera3D, Node3D);

public:
	Camera3D(float fovDegrees = 60.f, float nearClip = 0.1f, float farClip = 100.f);
	~Camera3D() override = default;

	// The aspect will be determined by the viewport so it is not a parameter of Camera3D
	// Instead, it will be passed in when GetCamera is called.
	CameraContext      GetCameraContext(float aspect) const;
	MathRaycastQuery3D BuildRaycastFromMouse(
		Vector2 const& mousePos, Vector2 const& viewportDimensions, float maxLength) const;

	void SetOrthogonal(float size, float nearClip = 0.f, float farClip = 1.f);
	void SetPerspective(float fovDegrees, float nearClip = 0.1f, float farClip = 100.f);

	CameraContext::Mode GetMode() const;
	float               GetNearClip() const;
	float               GetFarClip() const;
	float               GetFovDegrees() const;
	float               GetSize() const;

	void SetMode(CameraContext::Mode mode);
	void SetNearClip(float nearClip);
	void SetFarClip(float farClip);
	void SetFovDegrees(float fovDegrees);
	void SetSize(float size);

	static void BindMethods();

protected:
	void OnNotification(int notification);

private:
	CameraContext::Mode m_mode;
	float               m_nearClip;
	float               m_farClip;

	// Vertical FOV in degrees (perspective). Horizontal = fovY / aspect.
	float m_fovDegrees;

	// Vertical half-extent in world units (orthographic). Horizontal = size / aspect.
	float m_size;
};

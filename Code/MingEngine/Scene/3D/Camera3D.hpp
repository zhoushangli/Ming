#pragma once

#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Render/RenderContext.hpp"

// Camera node that defers aspect ratio to the viewport.
// Aspect is a viewport property, not a camera property — the Renderer
// combines it with this camera's CameraData when it builds the Projection.

// Both m_fovDegrees and m_size are measured on the vertical (Y) axis;
// the horizontal axis is derived from Y / aspect
// we do this because when we have different screen
// we want 16 : 9, 4 : 3, ... looks the same in height
class Camera3D : public Node3D
{
	MCLASS(Camera3D, Node3D);

public:
	Camera3D(float fovDegrees = 60.f, float nearClip = 0.1f, float farClip = 100.f);
	~Camera3D() override;

	// RID of the CameraData that mirrors this node on the RenderServer.
	// e.g. EnterTree binds it to the Viewport so the Renderer can look it up
	RID GetCameraRID() const { return m_cameraRID; }

	MathRaycastQuery3D BuildRaycastFromMouse(
		Vector2 const& mousePos, Vector2 const& viewportDimensions, float maxLength) const;

	void SetOrthogonal(float size, float nearClip = 0.f, float farClip = 1.f);
	void SetPerspective(float fovDegrees, float nearClip = 0.1f, float farClip = 100.f);

	CameraMode GetMode() const;
	float      GetNearClip() const;
	float      GetFarClip() const;
	float      GetFovDegrees() const;
	float      GetSize() const;

	void SetMode(CameraMode mode);
	void SetNearClip(float nearClip);
	void SetFarClip(float farClip);
	void SetFovDegrees(float fovDegrees);
	void SetSize(float size);

	static void BindMethods();

protected:
	void OnNotification(int notification);

private:
	// Pushes the world transform and projection parameters to the RenderServer.
	// e.g. SyncRenderData() runs on EnterTree and after every setter below
	void SyncRenderData();

private:
	RID m_cameraRID = RID::Invalid;

	CameraMode m_mode;
	float      m_nearClip;
	float      m_farClip;

	// Vertical FOV in degrees (perspective). Horizontal = fovY / aspect.
	float m_fovDegrees;

	// Vertical extent in world units (orthographic). Horizontal = size * aspect.
	float m_size;
};

#include "MingEngine/Scene/3D/Camera3D.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

Camera3D::Camera3D(float fovDegrees, float nearClip, float farClip)
	: m_mode(CameraMode::Perspective), m_nearClip(nearClip), m_farClip(farClip), m_fovDegrees(fovDegrees), m_size(1.f)
{
	m_cameraRID = g_engine->m_renderServer->CameraCreate();
}

Camera3D::~Camera3D()
{
	if (m_cameraRID.IsValid())
	{
		g_engine->m_renderServer->CameraFree(m_cameraRID);
		m_cameraRID = RID::Invalid;
	}
}

void Camera3D::BindMethods()
{
	ClassDatabase::BindMethod("SetNearClip", &Camera3D::SetNearClip, { "nearClip" });
	ClassDatabase::BindMethod("GetNearClip", &Camera3D::GetNearClip, {});
	ClassDatabase::BindMethod("SetFarClip", &Camera3D::SetFarClip, { "farClip" });
	ClassDatabase::BindMethod("GetFarClip", &Camera3D::GetFarClip, {});
	ClassDatabase::BindMethod("SetFovDegrees", &Camera3D::SetFovDegrees, { "fovDegrees" });
	ClassDatabase::BindMethod("GetFovDegrees", &Camera3D::GetFovDegrees, {});
	ClassDatabase::BindMethod("SetSize", &Camera3D::SetSize, { "size" });
	ClassDatabase::BindMethod("GetSize", &Camera3D::GetSize, {});

	PropertyInfo::UsageFlags const usage = PropertyInfo::UsageFlags::Default;
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "near_clip", PropertyInfo::Hint::None, "", usage),
		"SetNearClip",
		"GetNearClip");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "far_clip", PropertyInfo::Hint::None, "", usage),
		"SetFarClip",
		"GetFarClip");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "fov_degrees", PropertyInfo::Hint::None, "", usage),
		"SetFovDegrees",
		"GetFovDegrees");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Float, "size", PropertyInfo::Hint::None, "", usage), "SetSize", "GetSize");
}

void Camera3D::OnNotification(int notification)
{
	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server == nullptr)
	{
		return;
	}

	switch (notification)
	{
	case Notification_EnterTree:
	{
		if (m_data.m_viewport == nullptr)
		{
			break;
		}

		// 1) Join the Viewport camera list so scene queries can still find this camera.
		// 2) Bind the RID so the Viewport renders from this camera.
		m_data.m_viewport->RegisterWorldCamera(this);
		server->ViewportSetCamera(m_data.m_viewport->GetViewportRID(), m_cameraRID);
		SyncRenderData();
		break;
	}
	case Notification_ExitTree:
	{
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->UnregisterWorldCamera(this);
			server->ViewportFreeCamera(m_data.m_viewport->GetViewportRID(), m_cameraRID);
		}
		break;
	}
	case Notification_TransformChanged:
	{
		server->CameraSetTransform(m_cameraRID, GetWorldTransform());
		break;
	}
	}
}

void Camera3D::SyncRenderData()
{
	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server == nullptr || !m_cameraRID.IsValid())
	{
		return;
	}

	server->CameraSetTransform(m_cameraRID, GetWorldTransform());
	if (m_mode == CameraMode::Perspective)
	{
		server->CameraSetPerspective(m_cameraRID, m_fovDegrees, m_nearClip, m_farClip);
	}
	else
	{
		server->CameraSetOrthographic(m_cameraRID, m_size, m_nearClip, m_farClip);
	}
}

MathRaycastQuery3D Camera3D::BuildRaycastFromMouse(
	Vector2 const& mousePos, Vector2 const& viewportDimensions, float maxLength) const
{
	GUARANTEE_OR_DIE(m_mode == CameraMode::Perspective, "Camera3D mouse raycast only supports perspective cameras");

	MathRaycastQuery3D raycastInfo;
	raycastInfo.m_startPos      = GetWorldPosition();
	raycastInfo.m_forwardNormal = GetWorldForward();
	raycastInfo.m_maxLength     = maxLength;

	if (viewportDimensions.x <= 0.f || viewportDimensions.y <= 0.f)
	{
		return raycastInfo;
	}

	float const screenX        = 2.f * (mousePos.x / viewportDimensions.x) - 1.f;
	float const screenY        = 1.f - 2.f * (mousePos.y / viewportDimensions.y);
	float const aspect         = viewportDimensions.x / viewportDimensions.y;
	float const halfFovDegrees = m_fovDegrees * 0.5f;
	float const halfHeight     = Math::SinDegrees(halfFovDegrees) / Math::CosDegrees(halfFovDegrees);
	float const halfWidth      = halfHeight * aspect;

	Vector3 forward;
	Vector3 left;
	Vector3 up;
	GetWorldOrientation().GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

	raycastInfo.m_forwardNormal = (forward - left * screenX * halfWidth + up * screenY * halfHeight).GetNormalized();
	return raycastInfo;
}

void Camera3D::SetOrthogonal(float size, float nearClip, float farClip)
{
	m_mode     = CameraMode::Orthographic;
	m_size     = size;
	m_nearClip = nearClip;
	m_farClip  = farClip;
	SyncRenderData();
}

void Camera3D::SetPerspective(float fovDegrees, float nearClip, float farClip)
{
	m_mode       = CameraMode::Perspective;
	m_fovDegrees = fovDegrees;
	m_nearClip   = nearClip;
	m_farClip    = farClip;
	SyncRenderData();
}

CameraMode Camera3D::GetMode() const { return m_mode; }

void Camera3D::SetMode(CameraMode mode)
{
	m_mode = mode;
	SyncRenderData();
}

float Camera3D::GetNearClip() const { return m_nearClip; }

void Camera3D::SetNearClip(float nearClip)
{
	m_nearClip = nearClip;
	SyncRenderData();
}

float Camera3D::GetFarClip() const { return m_farClip; }

void Camera3D::SetFarClip(float farClip)
{
	m_farClip = farClip;
	SyncRenderData();
}

float Camera3D::GetFovDegrees() const { return m_fovDegrees; }

void Camera3D::SetFovDegrees(float fovDegrees)
{
	m_fovDegrees = fovDegrees;
	SyncRenderData();
}

float Camera3D::GetSize() const { return m_size; }

void Camera3D::SetSize(float size)
{
	m_size = size;
	SyncRenderData();
}

#include "MingEngine/Scene/3D/Light3D.hpp"

#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/EngineService/RenderService.hpp"

Light3D::Light3D() = default;
Light3D::Light3D(LightType type) : m_lightType(type) {}
Light3D::~Light3D() { FreeRenderLight(); }

void Light3D::BindMethods()
{
	ClassDatabase::BindMethod("SetIntensity", &Light3D::SetIntensity);
	ClassDatabase::BindMethod("GetIntensity", &Light3D::GetIntensity);
	ClassDatabase::BindMethod("SetColor", &Light3D::SetColor);
	ClassDatabase::BindMethod("GetColor", &Light3D::GetColor);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::Float,
			"intensity",
			PropertyInfo::Hint::None,
			"",
			PropertyInfo::UsageFlags::Default),
		"SetIntensity",
		"GetIntensity");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Color, "color", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default),
		"SetColor",
		"GetColor");
}

void Light3D::SetColor(Color const& color)
{
	m_color                      = color;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightColor(m_rid, color);
	}
}

void Light3D::SetIntensity(float intensity)
{
	m_intensity                  = intensity;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightIntensity(m_rid, intensity);
	}
}

Color Light3D::GetColor() const { return m_color; }
float Light3D::GetIntensity() const { return m_intensity; }

void Light3D::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	{
		RenderService* renderService = g_engineService->m_renderService;
		if (m_rid < 0 && renderService != nullptr)
		{
			m_rid = renderService->CreateLight(m_lightType);
			SyncRenderData();
		}
		break;
	}
	case Notification_ExitTree:
	{
		FreeRenderLight();
		break;
	}
	case Notification_TransformChanged:
	{
		RenderService* renderService = g_engineService->m_renderService;
		if (m_rid >= 0 && renderService != nullptr)
		{
			renderService->SetLightTransform(m_rid, GetWorldTransform());
		}
		break;
	}
	}
}

void Light3D::SyncRenderData()
{
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid < 0 || renderService == nullptr)
	{
		return;
	}

	renderService->SetLightTransform(m_rid, GetWorldTransform());
	renderService->SetLightColor(m_rid, m_color);
	renderService->SetLightIntensity(m_rid, m_intensity);
}

void Light3D::FreeRenderLight()
{
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->FreeLight(m_rid);
	}
	m_rid = -1;
}

OmniLight3D::OmniLight3D() : Light3D(LightType::Omni) {}

void OmniLight3D::SetRange(float range)
{
	m_range                      = range;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightRange(m_rid, range);
	}
}

void OmniLight3D::SetAttenuation(float attenuation)
{
	m_attenuation                = attenuation;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightAttenuation(m_rid, attenuation);
	}
}

void OmniLight3D::SyncRenderData()
{
	Light3D::SyncRenderData();
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightRange(m_rid, m_range);
		renderService->SetLightAttenuation(m_rid, m_attenuation);
	}
}

void OmniLight3D::BindMethods()
{
	ClassDatabase::BindMethod("SetRange", &OmniLight3D::SetRange);
	ClassDatabase::BindMethod("GetRange", &OmniLight3D::GetRange);
	ClassDatabase::BindMethod("SetAttenuation", &OmniLight3D::SetAttenuation);
	ClassDatabase::BindMethod("GetAttenuation", &OmniLight3D::GetAttenuation);
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "range", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default),
		"SetRange",
		"GetRange");
	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::Float,
			"attenuation",
			PropertyInfo::Hint::None,
			"",
			PropertyInfo::UsageFlags::Default),
		"SetAttenuation",
		"GetAttenuation");
}

DirectionalLight3D::DirectionalLight3D() : Light3D(LightType::Directional) {}

SpotLight3D::SpotLight3D() : Light3D(LightType::Spot) {}

void SpotLight3D::SetRange(float range)
{
	m_range                      = range;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightRange(m_rid, range);
	}
}

void SpotLight3D::SetAttenuation(float attenuation)
{
	m_attenuation                = attenuation;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightAttenuation(m_rid, attenuation);
	}
}

void SpotLight3D::SetSpotAngle(float angle)
{
	m_spotAngle = angle;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightSpotAngle(m_rid, angle);
	}
}

void SpotLight3D::SetSpotAttenuation(float attenuation)
{
	m_spotAttenuation = attenuation;
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightSpotAttenuation(m_rid, attenuation);
	}
}

void SpotLight3D::BindMethods()
{
	ClassDatabase::BindMethod("SetRange", &SpotLight3D::SetRange);
	ClassDatabase::BindMethod("GetRange", &SpotLight3D::GetRange);
	ClassDatabase::BindMethod("SetAttenuation", &SpotLight3D::SetAttenuation);
	ClassDatabase::BindMethod("GetAttenuation", &SpotLight3D::GetAttenuation);
	ClassDatabase::BindMethod("SetSpotAngle", &SpotLight3D::SetSpotAngle);
	ClassDatabase::BindMethod("GetSpotAngle", &SpotLight3D::GetSpotAngle);
	ClassDatabase::BindMethod("SetSpotAttenuation", &SpotLight3D::SetSpotAttenuation);
	ClassDatabase::BindMethod("GetSpotAttenuation", &SpotLight3D::GetSpotAttenuation);
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "range", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default),
		"SetRange",
		"GetRange");
	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::Float,
			"attenuation",
			PropertyInfo::Hint::None,
			"",
			PropertyInfo::UsageFlags::Default),
		"SetAttenuation",
		"GetAttenuation");
	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::Float,
			"spot_angle",
			PropertyInfo::Hint::None,
			"",
			PropertyInfo::UsageFlags::Default),
		"SetSpotAngle",
		"GetSpotAngle");
	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::Float,
			"spot_attenuation",
			PropertyInfo::Hint::None,
			"",
			PropertyInfo::UsageFlags::Default),
		"SetSpotAttenuation",
		"GetSpotAttenuation");
}

void SpotLight3D::SyncRenderData()
{
	Light3D::SyncRenderData();
	RenderService* renderService = g_engineService->m_renderService;
	if (m_rid >= 0 && renderService != nullptr)
	{
		renderService->SetLightRange(m_rid, m_range);
		renderService->SetLightAttenuation(m_rid, m_attenuation);
		renderService->SetLightSpotAngle(m_rid, m_spotAngle);
		renderService->SetLightSpotAttenuation(m_rid, m_spotAttenuation);
	}
}

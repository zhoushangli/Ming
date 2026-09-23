#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"

#include "MingEngine/Engine/Render/RenderServer.hpp"

Light3D::Light3D() : Light3D(LightType::Omni) {}
Light3D::Light3D(LightType type) : m_lightType(type)
{
	m_lightRID = g_engine->m_renderServer->LightCreate(type);
}
Light3D::~Light3D() { FreeRenderLight(); }

void Light3D::BindMethods()
{
	ClassDatabase::BindMethod("SetIntensity", &Light3D::SetIntensity, { "intensity" });
	ClassDatabase::BindMethod("GetIntensity", &Light3D::GetIntensity, {});
	ClassDatabase::BindMethod("SetColor", &Light3D::SetColor, { "color" });
	ClassDatabase::BindMethod("GetColor", &Light3D::GetColor, {});

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
	m_color                     = color;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetColor(m_lightRID, color);
	}
}

void Light3D::SetIntensity(float intensity)
{
	m_intensity                 = intensity;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetIntensity(m_lightRID, intensity);
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
		RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
		if (m_lightRID.IsValid() && renderService != nullptr)
		{
			SyncRenderData();
			renderService->InstanceSetBase(m_instanceRID, m_lightRID);
		}
		break;
	}
	}
}

void Light3D::SyncRenderData()
{
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (!m_lightRID.IsValid() || renderService == nullptr)
	{
		return;
	}

	renderService->LightSetColor(m_lightRID, m_color);
	renderService->LightSetIntensity(m_lightRID, m_intensity);
}

void Light3D::FreeRenderLight()
{
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightFree(m_lightRID);
	}
	m_lightRID = RID::Invalid;
}

OmniLight3D::OmniLight3D() : Light3D(LightType::Omni) {}

void OmniLight3D::SetRange(float range)
{
	m_range                     = range;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetRange(m_lightRID, range);
	}
}

void OmniLight3D::SetAttenuation(float attenuation)
{
	m_attenuation               = attenuation;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetAttenuation(m_lightRID, attenuation);
	}
}

void OmniLight3D::SyncRenderData()
{
	Light3D::SyncRenderData();
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetRange(m_lightRID, m_range);
		renderService->LightSetAttenuation(m_lightRID, m_attenuation);
	}
}

void OmniLight3D::BindMethods()
{
	ClassDatabase::BindMethod("SetRange", &OmniLight3D::SetRange, { "range" });
	ClassDatabase::BindMethod("GetRange", &OmniLight3D::GetRange, {});
	ClassDatabase::BindMethod("SetAttenuation", &OmniLight3D::SetAttenuation, { "attenuation" });
	ClassDatabase::BindMethod("GetAttenuation", &OmniLight3D::GetAttenuation, {});
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
	m_range                     = range;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetRange(m_lightRID, range);
	}
}

void SpotLight3D::SetAttenuation(float attenuation)
{
	m_attenuation               = attenuation;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetAttenuation(m_lightRID, attenuation);
	}
}

void SpotLight3D::SetSpotAngle(float angle)
{
	m_spotAngle                 = angle;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetSpotAngle(m_lightRID, angle);
	}
}

void SpotLight3D::SetSpotAttenuation(float attenuation)
{
	m_spotAttenuation           = attenuation;
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetSpotAttenuation(m_lightRID, attenuation);
	}
}

void SpotLight3D::BindMethods()
{
	ClassDatabase::BindMethod("SetRange", &SpotLight3D::SetRange, { "range" });
	ClassDatabase::BindMethod("GetRange", &SpotLight3D::GetRange, {});
	ClassDatabase::BindMethod("SetAttenuation", &SpotLight3D::SetAttenuation, { "attenuation" });
	ClassDatabase::BindMethod("GetAttenuation", &SpotLight3D::GetAttenuation, {});
	ClassDatabase::BindMethod("SetSpotAngle", &SpotLight3D::SetSpotAngle, { "angle" });
	ClassDatabase::BindMethod("GetSpotAngle", &SpotLight3D::GetSpotAngle, {});
	ClassDatabase::BindMethod("SetSpotAttenuation", &SpotLight3D::SetSpotAttenuation, { "attenuation" });
	ClassDatabase::BindMethod("GetSpotAttenuation", &SpotLight3D::GetSpotAttenuation, {});
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
	RenderServer* renderService = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (m_lightRID.IsValid() && renderService != nullptr)
	{
		renderService->LightSetRange(m_lightRID, m_range);
		renderService->LightSetAttenuation(m_lightRID, m_attenuation);
		renderService->LightSetSpotAngle(m_lightRID, m_spotAngle);
		renderService->LightSetSpotAttenuation(m_lightRID, m_spotAttenuation);
	}
}

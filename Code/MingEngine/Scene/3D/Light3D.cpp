#include "MingEngine/Scene/3D/Light3D.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"

Light3D::Light3D()  = default;
Light3D::~Light3D() = default;

void Light3D::BindMethods()
{
	ClassDatabase::BindMethod("SetDirection", &Light3D::SetDirection);
	ClassDatabase::BindMethod("GetDirection", &Light3D::GetDirection);
	ClassDatabase::BindMethod("SetIntensity", &Light3D::SetIntensity);
	ClassDatabase::BindMethod("GetIntensity", &Light3D::GetIntensity);
	ClassDatabase::BindMethod("SetPosition", &Light3D::SetPosition);
	ClassDatabase::BindMethod("GetPosition", &Light3D::GetPosition);
	ClassDatabase::BindMethod("SetRange", &Light3D::SetRange);
	ClassDatabase::BindMethod("GetRange", &Light3D::GetRange);

	PropertyInfo::UsageFlags const usage = PropertyInfo::UsageFlags::Default;
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Vec3, "direction", PropertyInfo::Hint::None, "", usage),
		"SetDirection",
		"GetDirection");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "intensity", PropertyInfo::Hint::None, "", usage),
		"SetIntensity",
		"GetIntensity");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Vec3, "position", PropertyInfo::Hint::None, "", usage),
		"SetPosition",
		"GetPosition");
	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Float, "range", PropertyInfo::Hint::None, "", usage),
		"SetRange",
		"GetRange");
}

void Light3D::SetColor(Rgba8 const& color) { m_lightInfo.m_color = color; }
void Light3D::SetDirection(Vec3 const& direction) { m_lightInfo.m_direction = direction.GetNormalized(); }
void Light3D::SetIntensity(float intensity) { m_lightInfo.m_intensity = intensity; }
void Light3D::SetPosition(Vec3 const& position) { m_lightInfo.m_position = position; }
void Light3D::SetRange(float range) { m_lightInfo.m_range = range; }

Rgba8            Light3D::GetColor() const { return m_lightInfo.m_color; }
float            Light3D::GetIntensity() const { return m_lightInfo.m_intensity; }
Vec3             Light3D::GetPosition() const { return m_lightInfo.m_position; }
Vec3             Light3D::GetDirection() const { return m_lightInfo.m_direction; }
float            Light3D::GetRange() const { return m_lightInfo.m_range; }
LightInfo const& Light3D::GetLightInfo() const { return m_lightInfo; }

void Light3D::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->RegisterLight(this);
		}
		break;
	case NotificationType::ExitTree:
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->UnregisterLight(this);
		}
		break;
	}
}

PointLight3D::PointLight3D() : Light3D() { m_lightInfo.m_type = LightType::POINT; }

DirectionalLight3D::DirectionalLight3D() : Light3D() { m_lightInfo.m_type = LightType::DIRECTIONAL; }

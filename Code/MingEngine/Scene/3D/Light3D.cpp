#include "MingEngine/Scene/3D/Light3D.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"

Light3D::Light3D() = default;
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

	ClassDatabase::PropertyInfo::PropertyUsageFlags const usage =
		ClassDatabase::PropertyInfo::PropertyUsageFlags::Default;
	ADD_PROPERTY(ClassDatabase::PropertyInfo(Variant::Type::Vec3, "direction", usage),
		"SetDirection",
		"GetDirection");
	ADD_PROPERTY(ClassDatabase::PropertyInfo(Variant::Type::Float, "intensity", usage),
		"SetIntensity",
		"GetIntensity");
	ADD_PROPERTY(ClassDatabase::PropertyInfo(Variant::Type::Vec3, "position", usage),
		"SetPosition",
		"GetPosition");
	ADD_PROPERTY(ClassDatabase::PropertyInfo(Variant::Type::Float, "range", usage), "SetRange", "GetRange");
}

void Light3D::SetColor(Rgba8 const& color)
{
	m_lightInfo.m_color = color;
}
void Light3D::SetDirection(Vec3 const& direction)
{
	m_lightInfo.m_direction = direction.GetNormalized();
}
void Light3D::SetIntensity(float intensity)
{
	m_lightInfo.m_intensity = intensity;
}
void Light3D::SetPosition(Vec3 const& position)
{
	m_lightInfo.m_position = position;
}
void Light3D::SetRange(float range)
{
	m_lightInfo.m_range = range;
}

Rgba8 Light3D::GetColor() const { return m_lightInfo.m_color; }
float Light3D::GetIntensity() const { return m_lightInfo.m_intensity; }
Vec3  Light3D::GetPosition() const { return m_lightInfo.m_position; }
Vec3  Light3D::GetDirection() const { return m_lightInfo.m_direction; }
float Light3D::GetRange() const { return m_lightInfo.m_range; }
LightInfo const& Light3D::GetLightInfo() const { return m_lightInfo; }

void Light3D::OnEnterTree()
{
	Node::OnEnterTree();
	// Lights are collected per Viewport instead of living in global Renderer state.
	if (m_data.m_viewport != nullptr)
	{
		m_data.m_viewport->RegisterLight(this);
	}
}

void Light3D::OnExitTree()
{
	// Remove the handle before Node propagation invalidates it.
	if (m_data.m_viewport != nullptr)
	{
		m_data.m_viewport->UnregisterLight(this);
	}
	Node::OnExitTree();
}

PointLight3D::PointLight3D() : Light3D()
{
	m_lightInfo.m_type = LightType::POINT;
}

DirectionalLight3D::DirectionalLight3D() : Light3D()
{
	m_lightInfo.m_type = LightType::DIRECTIONAL;
}

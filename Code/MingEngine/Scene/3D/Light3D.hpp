#pragma once

#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"

class Light3D : public Node3D
{
	MCLASS(Light3D, Node3D);

public:
	Light3D();
	Light3D(LightType type);
	~Light3D() override;

	void SetColor(Color const& color);
	void SetIntensity(float intensity);

	Color GetColor() const;
	float GetIntensity() const;

	static void BindMethods();

protected:
	void         OnNotification(int notification);
	virtual void SyncRenderData();
	void         FreeRenderLight();

protected:
	int       m_rid       = -1;
	LightType m_lightType = LightType::Omni;
	Color     m_color     = Color::White;
	float     m_intensity = 1.f;
};

class OmniLight3D : public Light3D
{
	MCLASS(OmniLight3D, Light3D);

public:
	OmniLight3D();
	~OmniLight3D() override = default;

	void  SetRange(float range);
	void  SetAttenuation(float attenuation);
	float GetRange() const { return m_range; }
	float GetAttenuation() const { return m_attenuation; }

protected:
	static void BindMethods();
	void        SyncRenderData() override;

private:
	float m_range       = 1.f;
	float m_attenuation = 1.f;
};

class DirectionalLight3D : public Light3D
{
	MCLASS(DirectionalLight3D, Light3D);

public:
	DirectionalLight3D();
	~DirectionalLight3D() override = default;
};

class SpotLight3D : public Light3D
{
	MCLASS(SpotLight3D, Light3D);

public:
	SpotLight3D();
	~SpotLight3D() override = default;

	void SetRange(float range);
	void SetAttenuation(float attenuation);
	void SetSpotAngle(float angle);
	void SetSpotAttenuation(float attenuation);

	float GetRange() const { return m_range; }
	float GetAttenuation() const { return m_attenuation; }
	float GetSpotAngle() const { return m_spotAngle; }
	float GetSpotAttenuation() const { return m_spotAttenuation; }

protected:
	static void BindMethods();
	void        SyncRenderData() override;

private:
	float m_range           = 1.f;
	float m_attenuation     = 1.f;
	float m_spotAngle       = 45.f;
	float m_spotAttenuation = 1.f;
};

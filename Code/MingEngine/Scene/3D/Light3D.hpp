#pragma once

#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Engine/Render/Rgba8.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"

class Light3D : public Node
{
	MCLASS(Light3D, Node);

public:
	Light3D();
	~Light3D() override;

	void SetColor(Rgba8 const& color);
	void SetDirection(Vec3 const& direction);
	void SetIntensity(float intensity);
	void SetPosition(Vec3 const& position);
	void SetRange(float range);

	Rgba8            GetColor() const;
	Vec3             GetDirection() const;
	float            GetIntensity() const;
	Vec3             GetPosition() const;
	float            GetRange() const;
	LightInfo const& GetLightInfo() const;

	static void BindMethods();

protected:
	void OnEnterTree() override;
	void OnExitTree() override;

protected:
	LightInfo m_lightInfo;
};

class PointLight3D : public Light3D
{
	MCLASS(PointLight3D, Light3D);

public:
	PointLight3D();
	~PointLight3D() override = default;
};

class DirectionalLight3D : public Light3D
{
	MCLASS(DirectionalLight3D, Light3D);

public:
	DirectionalLight3D();
	~DirectionalLight3D() override = default;
};

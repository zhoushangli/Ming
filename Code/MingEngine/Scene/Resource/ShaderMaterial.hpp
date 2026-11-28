#pragma once

#include "MingEngine/Scene/Resource/MaterialResource.hpp"

class ShaderMaterial : public MaterialResource
{
	MCLASS(ShaderMaterial, MaterialResource)

public:
	ShaderMaterial() = default;

	void SetShaderResource(Ref<ShaderResource> const& value) { MaterialResource::SetShaderResource(value); }
	Ref<ShaderResource> GetShaderResource() const { return MaterialResource::GetShaderResource(); }
	bool SetShaderParameter(String const& name, Variant const& value) { return SetParameter(name, value); }
	Variant GetShaderParameter(String const& name) { return GetParameter(name); }
	bool HasShaderParameterOverride(String const& name) { return HasParameterOverride(name); }
	bool ClearShaderParameterOverride(String const& name) { return ClearParameterOverride(name); }

	Variant GetPropertyState(std::string const& name) override;
	bool GetPropertyDefault(std::string const& name, Variant& value) override;
	uint64_t GetPropertyListRevision() override;

	void SetBlendMode(BlendMode value) { MaterialResource::SetBlendMode(value); }
	BlendMode GetBlendMode() const { return m_blendMode; }

	void SetDepthMode(DepthMode value) { MaterialResource::SetDepthMode(value); }
	DepthMode GetDepthMode() const { return m_depthMode; }

	void SetRasterizerMode(RasterizerMode value) { MaterialResource::SetRasterizerMode(value); }
	RasterizerMode GetRasterizerMode() const { return m_rasterizerMode; }

	void SetSamplerMode(SamplerMode value) { MaterialResource::SetSamplerMode(value); }
	SamplerMode GetSamplerMode() const { return m_samplerMode; }

protected:
	static void BindMethods();
	void GetDynamicPropertyList(std::vector<PropertyInfo>& properties) override;
	bool GetDynamicProperty(std::string const& name, Variant& value) override;
	bool SetDynamicProperty(std::string const& name, Variant const& value) override;

private:
	Ref<ShaderResource> m_propertyShader;
	uint64_t m_propertyShaderRevision = 0;
	uint64_t m_propertyListRevision = 0;
};

#pragma once

#include "MingEngine/Core/IO/Resource.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Engine/Render/RenderTypes.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <unordered_map>

class RenderServer;
class EditorUI;

class MaterialResource : public Resource
{
	MCLASS(MaterialResource, Resource)

	friend class RenderServer;
	friend class EditorUI;

public:
	~MaterialResource() override = default;

	MaterialResource(MaterialResource const&)            = delete;
	MaterialResource& operator=(MaterialResource const&) = delete;

	bool MoveFrom(Resource&& other) override;

protected:
	MaterialResource() = default;
	static void BindMethods() {}

	virtual void        PrepareShader() {}
	void                SetShaderResource(Ref<ShaderResource> const& value);
	Ref<ShaderResource> GetShaderResource() const { return m_shaderResource; }
	bool                SetParameter(String const& name, Variant const& value);
	Variant             GetParameter(String const& name);
	bool                HasParameterOverride(String const& name);
	bool                ClearParameterOverride(String const& name);

	void SetBlendMode(BlendMode value);
	void SetDepthMode(DepthMode value);
	void SetRasterizerMode(RasterizerMode value);
	void SetSamplerMode(SamplerMode value);

private:
	void UpdateShaderCache();
	void CopyFrom(MaterialResource& other);

protected:
	Ref<ShaderResource> m_shaderResource;
	BlendMode           m_blendMode      = BlendMode::OPAQUE;
	DepthMode           m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	RasterizerMode      m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	SamplerMode         m_samplerMode    = SamplerMode::POINT_CLAMP;

private:
	Ref<ShaderResource>                     m_parameterShader;
	uint64_t                                m_shaderRevision = 0;
	std::unordered_map<std::string, Variant> m_parameters;
	std::unordered_map<std::string, Variant> m_parameterOverrides;
};

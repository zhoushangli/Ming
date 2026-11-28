#pragma once

#include "MingEngine/Scene/Resource/MaterialResource.hpp"

class StandardMaterial3D : public MaterialResource
{
	MCLASS(StandardMaterial3D, MaterialResource)

public:
	StandardMaterial3D();

	void SetTint(Color const& value) { SetParameter("tint", value); }
	Color GetTint() const { return GetParameter("tint").As<Color>(); }

	void SetDiffuseTexture(Ref<TextureResource> const& value) { SetParameter("diffuse_texture", Variant::FromRef(value.Get())); }
	Ref<TextureResource> GetDiffuseTexture() const { return Ref<TextureResource>(dynamic_cast<TextureResource*>(GetParameter("diffuse_texture").As<Object*>())); }

	void SetNormalTexture(Ref<TextureResource> const& value) { SetParameter("normal_texture", Variant::FromRef(value.Get())); }
	Ref<TextureResource> GetNormalTexture() const { return Ref<TextureResource>(dynamic_cast<TextureResource*>(GetParameter("normal_texture").As<Object*>())); }

	void SetSGETexture(Ref<TextureResource> const& value) { SetParameter("sge_texture", Variant::FromRef(value.Get())); }
	Ref<TextureResource> GetSGETexture() const { return Ref<TextureResource>(dynamic_cast<TextureResource*>(GetParameter("sge_texture").As<Object*>())); }

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
	void PrepareShader() override;
};

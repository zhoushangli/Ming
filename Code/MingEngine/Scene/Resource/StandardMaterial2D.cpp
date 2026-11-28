#include "MingEngine/Scene/Resource/StandardMaterial2D.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"

StandardMaterial2D::StandardMaterial2D()
{
	m_shaderResource = CreateRef<ShaderResource>();
	m_shaderResource->SetUniforms(BuiltinShaders::GetMaterialUniforms(true));
	m_blendMode = BlendMode::ALPHA;
	m_depthMode = DepthMode::READ_ONLY_ALWAYS;
	m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	m_samplerMode = SamplerMode::BILINEAR_CLAMP;
}

void StandardMaterial2D::PrepareShader()
{
	if (m_shaderResource->IsEmpty())
	{
		SetShaderResource(g_engine->m_renderServer->GetBuiltinShaderResource("DefaultUI", BuiltinShaders::DefaultUI));
	}
}

void StandardMaterial2D::BindMethods()
{
	ClassDatabase::BindMethod("SetTint", &StandardMaterial2D::SetTint, { "value" });
	ClassDatabase::BindMethod("GetTint", &StandardMaterial2D::GetTint, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Color, "tint", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default), "SetTint", "GetTint");

	ClassDatabase::BindMethod("SetBlendMode", &StandardMaterial2D::SetBlendMode, { "value" });
	ClassDatabase::BindMethod("GetBlendMode", &StandardMaterial2D::GetBlendMode, {});
	ADD_GROUP("Render State");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "blend_mode", PropertyInfo::Hint::Enum, "Alpha:0,Additive:1,Opaque:2", PropertyInfo::UsageFlags::Default), "SetBlendMode", "GetBlendMode");

	ClassDatabase::BindMethod("SetTextureFilter", &StandardMaterial2D::SetTextureFilter, { "value" });
	ClassDatabase::BindMethod("GetTextureFilter", &StandardMaterial2D::GetTextureFilter, {});
	ClassDatabase::BindMethod("SetTextureRepeat", &StandardMaterial2D::SetTextureRepeat, { "value" });
	ClassDatabase::BindMethod("GetTextureRepeat", &StandardMaterial2D::GetTextureRepeat, {});
	ADD_GROUP("Sampling", "texture_");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "texture_filter", PropertyInfo::Hint::Enum, "Nearest:1,Linear:2", PropertyInfo::UsageFlags::Default), "SetTextureFilter", "GetTextureFilter");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "texture_repeat", PropertyInfo::Hint::Enum, "Disabled:1,Enabled:2", PropertyInfo::UsageFlags::Default), "SetTextureRepeat", "GetTextureRepeat");
}

void StandardMaterial2D::SetTextureFilter(TextureFilter value)
{
	ERR_FAIL_COND_MSG(value != TextureFilter::Nearest && value != TextureFilter::Linear, "Invalid TextureFilter.");
	bool repeat = GetTextureRepeat() == TextureRepeat::Enabled;
	SetSamplerMode(value == TextureFilter::Nearest
		? (repeat ? SamplerMode::POINT_WRAP : SamplerMode::POINT_CLAMP)
		: (repeat ? SamplerMode::BILINEAR_WRAP : SamplerMode::BILINEAR_CLAMP));
}

TextureFilter StandardMaterial2D::GetTextureFilter() const
{
	return m_samplerMode == SamplerMode::POINT_CLAMP || m_samplerMode == SamplerMode::POINT_WRAP
		? TextureFilter::Nearest : TextureFilter::Linear;
}

void StandardMaterial2D::SetTextureRepeat(TextureRepeat value)
{
	ERR_FAIL_COND_MSG(value != TextureRepeat::Disabled && value != TextureRepeat::Enabled, "Invalid TextureRepeat.");
	bool nearest = GetTextureFilter() == TextureFilter::Nearest;
	SetSamplerMode(value == TextureRepeat::Enabled
		? (nearest ? SamplerMode::POINT_WRAP : SamplerMode::BILINEAR_WRAP)
		: (nearest ? SamplerMode::POINT_CLAMP : SamplerMode::BILINEAR_CLAMP));
}

TextureRepeat StandardMaterial2D::GetTextureRepeat() const
{
	return m_samplerMode == SamplerMode::POINT_WRAP || m_samplerMode == SamplerMode::BILINEAR_WRAP
		? TextureRepeat::Enabled : TextureRepeat::Disabled;
}

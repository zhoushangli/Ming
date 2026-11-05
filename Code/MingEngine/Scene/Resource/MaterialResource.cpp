#include "MingEngine/Scene/Resource/MaterialResource.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <utility>

bool MaterialResource::CopyFrom(Resource&& other)
{
	// 1) Validate type
	MaterialResource* otherMaterial = dynamic_cast<MaterialResource*>(&other);
	if (otherMaterial == nullptr)
	{
		return false;
	}

	// 2) Move CPU data fields; a moved Ref leaves the source empty
	MoveBaseFrom(std::move(other));
	m_shaderResource   = std::move(otherMaterial->m_shaderResource);
	m_textureResources = std::move(otherMaterial->m_textureResources);
	m_tint             = otherMaterial->m_tint;
	m_blendMode        = otherMaterial->m_blendMode;
	m_depthMode        = otherMaterial->m_depthMode;
	m_rasterizerMode   = otherMaterial->m_rasterizerMode;
	m_samplerMode      = otherMaterial->m_samplerMode;

	return true;
}
void MaterialResource::BindMethods()
{
	ClassDatabase::BindMethod("SetShaderResource", &MaterialResource::SetShaderResource, { "value" });
	ClassDatabase::BindMethod("GetShaderResource", &MaterialResource::GetShaderResource, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "shader", PropertyInfo::Hint::ResourceType, "ShaderResource", PropertyInfo::UsageFlags::Default), "SetShaderResource", "GetShaderResource");

	ClassDatabase::BindMethod("SetDiffuseTexture", &MaterialResource::SetDiffuseTexture, { "value" });
	ClassDatabase::BindMethod("GetDiffuseTexture", &MaterialResource::GetDiffuseTexture, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "diffuse_texture", PropertyInfo::Hint::ResourceType, "TextureResource", PropertyInfo::UsageFlags::Default), "SetDiffuseTexture", "GetDiffuseTexture");

	ClassDatabase::BindMethod("SetNormalTexture", &MaterialResource::SetNormalTexture, { "value" });
	ClassDatabase::BindMethod("GetNormalTexture", &MaterialResource::GetNormalTexture, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "normal_texture", PropertyInfo::Hint::ResourceType, "TextureResource", PropertyInfo::UsageFlags::Default), "SetNormalTexture", "GetNormalTexture");

	ClassDatabase::BindMethod("SetSGETexture", &MaterialResource::SetSGETexture, { "value" });
	ClassDatabase::BindMethod("GetSGETexture", &MaterialResource::GetSGETexture, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "sge_texture", PropertyInfo::Hint::ResourceType, "TextureResource", PropertyInfo::UsageFlags::Default), "SetSGETexture", "GetSGETexture");

	ClassDatabase::BindMethod("SetTint", &MaterialResource::SetTint, { "value" });
	ClassDatabase::BindMethod("GetTint", &MaterialResource::GetTint, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Color, "tint", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default), "SetTint", "GetTint");

	ClassDatabase::BindMethod("SetBlendMode", &MaterialResource::SetBlendMode, { "value" });
	ClassDatabase::BindMethod("GetBlendMode", &MaterialResource::GetBlendMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "blend_mode", PropertyInfo::Hint::Enum, "ALPHA,ADDITIVE,OPAQUE", PropertyInfo::UsageFlags::Default), "SetBlendMode", "GetBlendMode");

	ClassDatabase::BindMethod("SetDepthMode", &MaterialResource::SetDepthMode, { "value" });
	ClassDatabase::BindMethod("GetDepthMode", &MaterialResource::GetDepthMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "depth_mode", PropertyInfo::Hint::Enum, "DISABLED,READ_ONLY_ALWAYS,READ_ONLY_LESS_EQUAL,READ_WRITE_LESS_EQUAL", PropertyInfo::UsageFlags::Default), "SetDepthMode", "GetDepthMode");

	ClassDatabase::BindMethod("SetRasterizerMode", &MaterialResource::SetRasterizerMode, { "value" });
	ClassDatabase::BindMethod("GetRasterizerMode", &MaterialResource::GetRasterizerMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "rasterizer_mode", PropertyInfo::Hint::Enum, "SOLID_CULL_NONE,SOLID_CULL_BACK,WIREFRAME_CULL_NONE,WIREFRAME_CULL_BACK", PropertyInfo::UsageFlags::Default), "SetRasterizerMode", "GetRasterizerMode");

	ClassDatabase::BindMethod("SetSamplerMode", &MaterialResource::SetSamplerMode, { "value" });
	ClassDatabase::BindMethod("GetSamplerMode", &MaterialResource::GetSamplerMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "sampler_mode", PropertyInfo::Hint::Enum, "POINT_CLAMP,BILINEAR_CLAMP,POINT_WRAP,BILINEAR_WRAP", PropertyInfo::UsageFlags::Default), "SetSamplerMode", "GetSamplerMode");

}

void MaterialResource::SetBlendMode(BlendMode value)
{
	ERR_FAIL_COND_MSG(static_cast<int>(value) < 0 || value >= BlendMode::COUNT, "Invalid BlendMode.");
	m_blendMode = value;
}

void MaterialResource::SetDepthMode(DepthMode value)
{
	ERR_FAIL_COND_MSG(static_cast<int>(value) < 0 || value >= DepthMode::COUNT, "Invalid DepthMode.");
	m_depthMode = value;
}

void MaterialResource::SetRasterizerMode(RasterizerMode value)
{
	ERR_FAIL_COND_MSG(static_cast<int>(value) < 0 || value >= RasterizerMode::COUNT, "Invalid RasterizerMode.");
	m_rasterizerMode = value;
}

void MaterialResource::SetSamplerMode(SamplerMode value)
{
	ERR_FAIL_COND_MSG(static_cast<int>(value) < 0 || value >= SamplerMode::COUNT, "Invalid SamplerMode.");
	m_samplerMode = value;
}

#include "MingEngine/Scene/Resource/StandardMaterial3D.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"

StandardMaterial3D::StandardMaterial3D()
{
	m_shaderResource = CreateRef<ShaderResource>();
	m_shaderResource->SetUniforms(BuiltinShaders::GetMaterialUniforms(false));
}

void StandardMaterial3D::PrepareShader()
{
	if (m_shaderResource->IsEmpty())
	{
		SetShaderResource(g_engine->m_renderServer->GetBuiltinShaderResource("DefaultLit", BuiltinShaders::DefaultLit));
	}
}

void StandardMaterial3D::BindMethods()
{
	ClassDatabase::BindMethod("SetDiffuseTexture", &StandardMaterial3D::SetDiffuseTexture, { "value" });
	ClassDatabase::BindMethod("GetDiffuseTexture", &StandardMaterial3D::GetDiffuseTexture, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "diffuse_texture", PropertyInfo::Hint::ResourceType, "TextureResource", PropertyInfo::UsageFlags::Default), "SetDiffuseTexture", "GetDiffuseTexture");

	ClassDatabase::BindMethod("SetNormalTexture", &StandardMaterial3D::SetNormalTexture, { "value" });
	ClassDatabase::BindMethod("GetNormalTexture", &StandardMaterial3D::GetNormalTexture, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "normal_texture", PropertyInfo::Hint::ResourceType, "TextureResource", PropertyInfo::UsageFlags::Default), "SetNormalTexture", "GetNormalTexture");

	ClassDatabase::BindMethod("SetSGETexture", &StandardMaterial3D::SetSGETexture, { "value" });
	ClassDatabase::BindMethod("GetSGETexture", &StandardMaterial3D::GetSGETexture, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "sge_texture", PropertyInfo::Hint::ResourceType, "TextureResource", PropertyInfo::UsageFlags::Default), "SetSGETexture", "GetSGETexture");

	ClassDatabase::BindMethod("SetTint", &StandardMaterial3D::SetTint, { "value" });
	ClassDatabase::BindMethod("GetTint", &StandardMaterial3D::GetTint, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Color, "tint", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default), "SetTint", "GetTint");

	ClassDatabase::BindMethod("SetBlendMode", &StandardMaterial3D::SetBlendMode, { "value" });
	ClassDatabase::BindMethod("GetBlendMode", &StandardMaterial3D::GetBlendMode, {});
	ADD_GROUP("Render State");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "blend_mode", PropertyInfo::Hint::Enum, "Alpha:0,Additive:1,Opaque:2", PropertyInfo::UsageFlags::Default), "SetBlendMode", "GetBlendMode");

	ClassDatabase::BindMethod("SetDepthMode", &StandardMaterial3D::SetDepthMode, { "value" });
	ClassDatabase::BindMethod("GetDepthMode", &StandardMaterial3D::GetDepthMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "depth_mode", PropertyInfo::Hint::Enum, "Disabled:0,Read Only Always:1,Read Only Less or Equal:2,Read Write Less or Equal:3", PropertyInfo::UsageFlags::Default), "SetDepthMode", "GetDepthMode");

	ClassDatabase::BindMethod("SetRasterizerMode", &StandardMaterial3D::SetRasterizerMode, { "value" });
	ClassDatabase::BindMethod("GetRasterizerMode", &StandardMaterial3D::GetRasterizerMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "rasterizer_mode", PropertyInfo::Hint::Enum, "Solid Cull None:0,Solid Cull Back:1,Wireframe Cull None:2,Wireframe Cull Back:3", PropertyInfo::UsageFlags::Default), "SetRasterizerMode", "GetRasterizerMode");

	ClassDatabase::BindMethod("SetSamplerMode", &StandardMaterial3D::SetSamplerMode, { "value" });
	ClassDatabase::BindMethod("GetSamplerMode", &StandardMaterial3D::GetSamplerMode, {});
	ADD_GROUP("Sampling");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "sampler_mode", PropertyInfo::Hint::Enum, "Nearest Clamp:0,Linear Clamp:1,Nearest Repeat:2,Linear Repeat:3", PropertyInfo::UsageFlags::Default), "SetSamplerMode", "GetSamplerMode");
}

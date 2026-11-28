#include "MingEngine/Scene/Resource/ShaderMaterial.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

void ShaderMaterial::BindMethods()
{
	ClassDatabase::BindMethod("SetShaderResource", &ShaderMaterial::SetShaderResource, { "value" });
	ClassDatabase::BindMethod("GetShaderResource", &ShaderMaterial::GetShaderResource, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::ObjectPtr, "shader", PropertyInfo::Hint::ResourceType, "ShaderResource", PropertyInfo::UsageFlags::Default), "SetShaderResource", "GetShaderResource");

	ClassDatabase::BindMethod("SetBlendMode", &ShaderMaterial::SetBlendMode, { "value" });
	ClassDatabase::BindMethod("GetBlendMode", &ShaderMaterial::GetBlendMode, {});
	ADD_GROUP("Render State");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "blend_mode", PropertyInfo::Hint::Enum, "Alpha:0,Additive:1,Opaque:2", PropertyInfo::UsageFlags::Default), "SetBlendMode", "GetBlendMode");

	ClassDatabase::BindMethod("SetDepthMode", &ShaderMaterial::SetDepthMode, { "value" });
	ClassDatabase::BindMethod("GetDepthMode", &ShaderMaterial::GetDepthMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "depth_mode", PropertyInfo::Hint::Enum, "Disabled:0,Read Only Always:1,Read Only Less or Equal:2,Read Write Less or Equal:3", PropertyInfo::UsageFlags::Default), "SetDepthMode", "GetDepthMode");

	ClassDatabase::BindMethod("SetRasterizerMode", &ShaderMaterial::SetRasterizerMode, { "value" });
	ClassDatabase::BindMethod("GetRasterizerMode", &ShaderMaterial::GetRasterizerMode, {});
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "rasterizer_mode", PropertyInfo::Hint::Enum, "Solid Cull None:0,Solid Cull Back:1,Wireframe Cull None:2,Wireframe Cull Back:3", PropertyInfo::UsageFlags::Default), "SetRasterizerMode", "GetRasterizerMode");

	ClassDatabase::BindMethod("SetSamplerMode", &ShaderMaterial::SetSamplerMode, { "value" });
	ClassDatabase::BindMethod("GetSamplerMode", &ShaderMaterial::GetSamplerMode, {});
	ADD_GROUP("Sampling");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Int, "sampler_mode", PropertyInfo::Hint::Enum, "Nearest Clamp:0,Linear Clamp:1,Nearest Repeat:2,Linear Repeat:3", PropertyInfo::UsageFlags::Default), "SetSamplerMode", "GetSamplerMode");

	ClassDatabase::BindMethod("SetShaderParameter", &ShaderMaterial::SetShaderParameter, { "name", "value" });
	ClassDatabase::BindMethod("GetShaderParameter", &ShaderMaterial::GetShaderParameter, { "name" });
}

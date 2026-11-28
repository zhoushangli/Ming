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

void ShaderMaterial::GetDynamicPropertyList(std::vector<PropertyInfo>& properties)
{
	auto shader = GetShaderResource();
	if (!shader.IsValid() || shader->GetUniforms().empty())
	{
		return;
	}

	properties.emplace_back(Variant::Type::Empty, "Shader Parameters", PropertyInfo::Hint::None,
		"shader_parameter/", PropertyInfo::UsageFlags::Group);
	std::string group;
	for (auto const& uniform : shader->GetUniforms())
	{
		if (uniform.m_group != group)
		{
			group = uniform.m_group;
			properties.emplace_back(Variant::Type::Empty, group.empty() ? "Shader Parameters" : group,
				PropertyInfo::Hint::None, "shader_parameter/", PropertyInfo::UsageFlags::Group);
		}

		auto usage = PropertyInfo::UsageFlags::Inspector;
		if (HasShaderParameterOverride(uniform.m_name))
		{
			usage = usage | PropertyInfo::UsageFlags::Storage;
		}
		properties.emplace_back(uniform.m_defaultValue.GetType(), "shader_parameter/" + uniform.m_name,
			uniform.m_hint, uniform.m_hintData, usage);
	}
}

bool ShaderMaterial::GetDynamicProperty(std::string const& name, Variant& value)
{
	Variant defaultValue;
	if (name.compare(0, 17, "shader_parameter/") != 0 || !GetPropertyDefault(name, defaultValue))
	{
		return false;
	}
	value = GetShaderParameter(name.substr(17));
	return true;
}

bool ShaderMaterial::SetDynamicProperty(std::string const& name, Variant const& value)
{
	Variant defaultValue;
	if (name.compare(0, 17, "shader_parameter/") != 0 || !GetPropertyDefault(name, defaultValue))
	{
		return false;
	}
	return value.IsEmpty() ? ClearShaderParameterOverride(name.substr(17)) : SetShaderParameter(name.substr(17), value);
}

Variant ShaderMaterial::GetPropertyState(std::string const& name)
{
	if (name.compare(0, 17, "shader_parameter/") == 0 && !HasShaderParameterOverride(name.substr(17)))
	{
		return {};
	}
	return GetProperty(name);
}

bool ShaderMaterial::GetPropertyDefault(std::string const& name, Variant& value)
{
	if (name.compare(0, 17, "shader_parameter/") != 0)
	{
		return Object::GetPropertyDefault(name, value);
	}
	auto shader = GetShaderResource();
	if (shader.IsValid())
	{
		for (auto const& uniform : shader->GetUniforms())
		{
			if (uniform.m_name == name.substr(17))
			{
				value = uniform.m_defaultValue;
				return true;
			}
		}
	}
	return false;
}

uint64_t ShaderMaterial::GetPropertyListRevision()
{
	auto shader = GetShaderResource();
	uint64_t revision = shader.IsValid() ? shader->GetRevision() : 0;
	if (shader != m_propertyShader || revision != m_propertyShaderRevision)
	{
		m_propertyShader = shader;
		m_propertyShaderRevision = revision;
		++m_propertyListRevision;
	}
	return m_propertyListRevision;
}

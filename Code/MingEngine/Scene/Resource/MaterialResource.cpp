#include "MingEngine/Scene/Resource/MaterialResource.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <utility>

void MaterialResource::CopyFrom(MaterialResource& other)
{
	other.UpdateShaderCache();
	m_shaderResource     = other.m_shaderResource;
	m_parameterShader    = other.m_parameterShader;
	m_shaderRevision     = other.m_shaderRevision;
	m_parameters         = other.m_parameters;
	m_parameterOverrides = other.m_parameterOverrides;
	m_blendMode          = other.m_blendMode;
	m_depthMode          = other.m_depthMode;
	m_rasterizerMode     = other.m_rasterizerMode;
	m_samplerMode        = other.m_samplerMode;
}

bool MaterialResource::MoveFrom(Resource&& other)
{
	auto* material = dynamic_cast<MaterialResource*>(&other);
	if (material == nullptr || material == this || material->GetClassName() != GetClassName())
	{
		return false;
	}

	material->UpdateShaderCache();
	m_shaderResource     = material->m_shaderResource;
	m_parameterShader    = material->m_parameterShader;
	m_shaderRevision     = material->m_shaderRevision;
	m_parameters         = material->m_parameters;
	m_parameterOverrides = material->m_parameterOverrides;
	m_blendMode          = material->m_blendMode;
	m_depthMode          = material->m_depthMode;
	m_rasterizerMode     = material->m_rasterizerMode;
	m_samplerMode        = material->m_samplerMode;

	m_name               = std::move(material->m_name);
	m_virtualPath        = std::move(material->m_virtualPath);
	m_sourceFilePath     = std::move(material->m_sourceFilePath);

	return true;
}

void MaterialResource::SetShaderResource(Ref<ShaderResource> const& value)
{
	m_shaderResource = value;
	UpdateShaderCache();
}

void MaterialResource::UpdateShaderCache()
{
	uint64_t revision = m_shaderResource.IsValid() ? m_shaderResource->GetRevision() : 0;
	if (m_parameterShader == m_shaderResource && m_shaderRevision == revision)
	{
		return;
	}

	m_parameterShader = m_shaderResource;
	m_shaderRevision  = revision;
	auto previous     = std::move(m_parameterOverrides);
	m_parameterOverrides.clear();
	m_parameters.clear();
	if (m_shaderResource.IsValid())
	{
		for (MShaderUniform const& uniform : m_shaderResource->GetUniforms())
		{
			auto found = previous.find(uniform.m_name);
			if (found != previous.end() && uniform.m_binding == MShaderUniform::Binding::ConstantBuffer)
			{
				if (uniform.m_defaultValue.Is<Color>() && found->second.Is<Vector4>())
				{
					auto const& v = found->second.As<Vector4>();
					found->second = Color(v.x, v.y, v.z, v.w);
				}
				else if (uniform.m_defaultValue.Is<Vector4>() && found->second.Is<Color>())
				{
					auto const& c = found->second.As<Color>();
					found->second = Vector4(c.r, c.g, c.b, c.a);
				}
			}
			if (found != previous.end() && found->second.GetType() == uniform.m_defaultValue.GetType())
			{
				m_parameterOverrides.emplace(uniform.m_name, found->second);
			}
			m_parameters.emplace(
				uniform.m_name,
				found != previous.end() && found->second.GetType() == uniform.m_defaultValue.GetType()
					? found->second
					: uniform.m_defaultValue);
		}
	}
}

bool MaterialResource::SetParameter(String const& name, Variant const& value)
{
	UpdateShaderCache();
	auto found = m_parameters.find(name.ToUtf8());
	if (found == m_parameters.end() || found->second.GetType() != value.GetType())
	{
		ERR_PRINT("Unknown material parameter or mismatched type: " + name.ToUtf8());
		return false;
	}
	if (value.GetType() == Variant::Type::ObjectPtr && value.As<Object*>() != nullptr
		&& dynamic_cast<TextureResource*>(value.As<Object*>()) == nullptr)
	{
		ERR_PRINT("Material texture parameter requires a TextureResource.");
		return false;
	}
	m_parameterOverrides[name.ToUtf8()] = value.GetType() == Variant::Type::ObjectPtr ? Variant::FromRef(value.As<Object*>()) : value;
	if (found->second != value)
	{
		found->second = value.GetType() == Variant::Type::ObjectPtr ? Variant::FromRef(value.As<Object*>()) : value;
	}
	return true;
}

Variant MaterialResource::GetParameter(String const& name)
{
	UpdateShaderCache();
	auto found = m_parameters.find(name.ToUtf8());
	if (found == m_parameters.end())
	{
		ERR_PRINT("Unknown material parameter: " + name.ToUtf8());
		return Variant();
	}
	return found->second;
}

bool MaterialResource::HasParameterOverride(String const& name)
{
	UpdateShaderCache();
	return m_parameterOverrides.find(name.ToUtf8()) != m_parameterOverrides.end();
}

bool MaterialResource::ClearParameterOverride(String const& name)
{
	UpdateShaderCache();
	if (m_shaderResource.IsValid())
	{
		for (auto const& uniform : m_shaderResource->GetUniforms())
		{
			if (uniform.m_name == name.ToUtf8())
			{
				m_parameterOverrides.erase(uniform.m_name);
				m_parameters[uniform.m_name] = uniform.m_defaultValue;
				return true;
			}
		}
	}
	return false;
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

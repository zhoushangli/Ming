#include "MingEngine/Scene/Resource/MaterialResource.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Render/ConstantBuffer.hpp"
#include "MingEngine/Engine/Render/RenderBackend.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

MaterialResource::~MaterialResource()
{
	delete m_parameterBuffer;
}

void MaterialResource::CopyValuesFrom(MaterialResource const& other)
{
	other.SynchronizeShader();
	delete m_parameterBuffer;
	m_parameterBuffer = nullptr;
	m_shaderResource = other.m_shaderResource;
	m_parameterShader = other.m_parameterShader;
	m_shaderRevision = other.m_shaderRevision;
	m_parameters = other.m_parameters;
	m_parameterBytes = other.m_parameterBytes;
	m_parametersDirty = true;
	m_blendMode = other.m_blendMode;
	m_depthMode = other.m_depthMode;
	m_rasterizerMode = other.m_rasterizerMode;
	m_samplerMode = other.m_samplerMode;
}

bool MaterialResource::CopyFrom(Resource&& other)
{
	auto* material = dynamic_cast<MaterialResource*>(&other);
	if (material == nullptr || material == this || material->GetClassName() != GetClassName())
	{
		return false;
	}

	CopyValuesFrom(*material);
	MoveBaseFrom(std::move(other));
	return true;
}

void MaterialResource::SetShaderResource(Ref<ShaderResource> const& value)
{
	m_shaderResource = value;
	SynchronizeShader();
}

void MaterialResource::SynchronizeShader() const
{
	uint64_t revision = m_shaderResource.IsValid() ? m_shaderResource->GetRevision() : 0;
	if (m_parameterShader == m_shaderResource && m_shaderRevision == revision)
	{
		return;
	}

	delete m_parameterBuffer;
	m_parameterBuffer = nullptr;
	m_parameterShader = m_shaderResource;
	m_shaderRevision = revision;
	auto previous = std::move(m_parameters);
	m_parameters.clear();
	m_parameterBytes.clear();
	m_parametersDirty = true;
	if (m_shaderResource.IsValid())
	{
		for (MShaderUniform const& uniform : m_shaderResource->GetUniforms())
		{
			auto found = previous.find(uniform.m_name);
			m_parameters.emplace(uniform.m_name,
				found != previous.end() && found->second.GetType() == uniform.m_defaultValue.GetType()
					? found->second : uniform.m_defaultValue);
			if (uniform.m_binding == MShaderUniform::Binding::ConstantBuffer)
			{
				m_parameterBytes.resize(std::max(m_parameterBytes.size(), size_t(uniform.m_offset) + 16), 0);
			}
		}
	}
}

bool MaterialResource::SetParameter(String const& name, Variant const& value)
{
	SynchronizeShader();
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
	if (found->second != value)
	{
		found->second = value.GetType() == Variant::Type::ObjectPtr ? Variant::FromRef(value.As<Object*>()) : value;
		m_parametersDirty = true;
	}
	return true;
}

Variant MaterialResource::GetParameter(String const& name) const
{
	SynchronizeShader();
	auto found = m_parameters.find(name.ToUtf8());
	if (found == m_parameters.end())
	{
		ERR_PRINT("Unknown material parameter: " + name.ToUtf8());
		return Variant();
	}
	return found->second;
}

void MaterialResource::BindParameters(RenderBackend& backend)
{
	SynchronizeShader();
	if (m_parameterBytes.empty())
	{
		backend.BindConstantBuffer(nullptr, 6);
		return;
	}
	if (m_parameterBuffer == nullptr)
	{
		m_parameterBuffer = backend.CreateConstantBuffer(static_cast<unsigned int>(m_parameterBytes.size()));
		m_parametersDirty = true;
	}
	if (m_parametersDirty)
	{
		for (MShaderUniform const& uniform : m_shaderResource->GetUniforms())
		{
			if (uniform.m_binding != MShaderUniform::Binding::ConstantBuffer)
			{
				continue;
			}
			Variant const& value = m_parameters.at(uniform.m_name);
			void const* data = nullptr;
			switch (value.GetType())
			{
			case Variant::Type::Float:
				data = &value.As<float>();
				break;
			case Variant::Type::Vector2:
				data = &value.As<Vector2>();
				break;
			case Variant::Type::Vector3:
				data = &value.As<Vector3>();
				break;
			case Variant::Type::Vector4:
				data = &value.As<Vector4>();
				break;
			default:
				break;
			}
			if (data != nullptr)
			{
				std::memcpy(m_parameterBytes.data() + uniform.m_offset, data, uniform.m_components * sizeof(float));
			}
		}
		backend.CopyCPUToGPU(m_parameterBytes.data(), static_cast<unsigned int>(m_parameterBytes.size()), m_parameterBuffer);
		m_parametersDirty = false;
	}
	backend.BindConstantBuffer(m_parameterBuffer, 6);
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

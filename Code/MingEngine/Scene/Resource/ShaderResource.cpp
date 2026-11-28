#include "MingEngine/Scene/Resource/ShaderResource.hpp"

#include <utility>

ShaderResource::~ShaderResource()
{
	delete m_shader;
	m_shader = nullptr;
}

bool ShaderResource::MoveFrom(Resource&& other)
{
	ShaderResource* otherShader = dynamic_cast<ShaderResource*>(&other);
	if (otherShader == nullptr)
	{
		return false;
	}

	m_name           = std::move(otherShader->m_name);
	m_virtualPath    = std::move(otherShader->m_virtualPath);
	m_sourceFilePath = std::move(otherShader->m_sourceFilePath);

	delete m_shader;
	m_shader = nullptr;
	std::swap(m_shader, otherShader->m_shader);
	m_uniforms = std::move(otherShader->m_uniforms);
	++m_revision;

	return true;
}

void ShaderResource::SetUniforms(std::vector<MShaderUniform> uniforms)
{
	for (auto& uniform : uniforms)
	{
		if (uniform.m_hint == PropertyInfo::Hint::Color && uniform.m_defaultValue.Is<Vector4>())
		{
			auto const& v = uniform.m_defaultValue.As<Vector4>();
			uniform.m_defaultValue = Color(v.x, v.y, v.z, v.w);
		}
		else if (uniform.m_hint != PropertyInfo::Hint::Color && uniform.m_defaultValue.Is<Color>()
			&& uniform.m_binding == MShaderUniform::Binding::ConstantBuffer)
		{
			auto const& c = uniform.m_defaultValue.As<Color>();
			uniform.m_defaultValue = Vector4(c.r, c.g, c.b, c.a);
		}
	}

	m_uniforms = std::move(uniforms);
	++m_revision;
}

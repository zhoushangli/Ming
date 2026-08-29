#include "MingEngine/Scene/Resource/ShaderResource.hpp"

#include <utility>

ShaderResource::~ShaderResource()
{
	delete m_shader;
	m_shader = nullptr;
}

bool ShaderResource::CopyFrom(Resource&& other)
{
	ShaderResource* otherShader = dynamic_cast<ShaderResource*>(&other);
	if (otherShader == nullptr)
	{
		return false;
	}

	MoveBaseFrom(std::move(other));

	delete m_shader;
	m_shader = nullptr;
	std::swap(m_shader, otherShader->m_shader);

	return true;
}

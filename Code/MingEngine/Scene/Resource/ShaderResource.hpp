#pragma once

#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Engine/Render/Shader.hpp"

class ShaderResource : public Resource
{
	MCLASS(ShaderResource, Resource);

public:
	ShaderResource()                                      = default;
	ShaderResource(ShaderResource const& copy)            = delete;
	ShaderResource& operator=(ShaderResource const& copy) = delete;
	~ShaderResource() override;

	bool IsEmpty() const { return m_shader == nullptr; }
	bool CopyFrom(Resource&& other) override;

	Shader* GetShader() const { return m_shader; }
	void    SetShader(Shader* shader) { m_shader = shader; }

protected:
	static void BindMethods() {}

private:
	Shader* m_shader = nullptr;
};

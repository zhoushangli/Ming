#pragma once

#include "MingEngine/Core/IO/Resource.hpp"
#include "MingEngine/Engine/Render/Shader.hpp"
#include "MingEngine/Engine/Render/MShaderParser.hpp"

class ShaderResource : public Resource
{
	MCLASS(ShaderResource, Resource);

public:
	ShaderResource() = default;
	~ShaderResource() override;

	ShaderResource(ShaderResource const& copy)            = delete;
	ShaderResource& operator=(ShaderResource const& copy) = delete;

	bool IsEmpty() const { return m_shader == nullptr; }

	bool MoveFrom(Resource&& other) override;

	Shader* GetShader() const { return m_shader; }

	void SetShader(Shader* shader) { m_shader = shader; ++m_revision; }
	void SetUniforms(std::vector<MShaderUniform> uniforms);
	std::vector<MShaderUniform> const& GetUniforms() const { return m_uniforms; }
	uint64_t GetRevision() const { return m_revision; }

protected:
	static void BindMethods() {}

private:
	Shader* m_shader = nullptr;
	std::vector<MShaderUniform> m_uniforms;
	uint64_t m_revision = 0;
};

#include "Engine/Renderer/PostProcessPass.hpp"

#include "Engine/Core/Engine.hpp"

PostProcessPass::PostProcessPass(std::string const& passName, std::string const& postProcessShaderName)
	: m_name(passName), m_wideName(passName.begin(), passName.end())
{
	m_postProcessShader = g_engine->m_renderer->CreateOrGetShader(postProcessShaderName.c_str());
}

PostProcessPass::~PostProcessPass() {}

bool PostProcessPass::HasCustomOutput() const
{
	{
		return m_customOutput.IsValid();
	};
}

bool PostProcessPass::HasCustomInputs() const
{
	for (const auto& input : m_customInputs)
	{
		if (input.IsValid())
		{
			return true;
		}
	}
	return false;
}

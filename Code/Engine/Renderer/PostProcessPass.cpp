#include "Engine/Renderer/PostProcessPass.hpp"

#include "Engine/Core/Engine.hpp"

PostProcessPass::PostProcessPass(std::string const& passName, std::string const& postProcessShaderName, IntVec2 renderTargetSize)
	: m_name(passName), m_wideName(passName.begin(), passName.end()), m_renderTargetSize(renderTargetSize)
{
	m_postProcessShader = g_engine->m_renderer->CreateOrGetShader(postProcessShaderName.c_str());
}

PostProcessPass::~PostProcessPass() {}

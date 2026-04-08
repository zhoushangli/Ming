#include "Engine/Renderer/PostProcessPass.hpp"

#include "Engine/Core/Engine.hpp"

PostProcessPass::PostProcessPass(
    std::string const& passName, 
    std::string const& postProcessShaderName) : 
    m_name(passName),
    m_wideName(passName.begin(), passName.end())
{
    m_postProcessShader = g_engine->m_renderer->CreateShader(postProcessShaderName.c_str());
}

PostProcessPass::~PostProcessPass()
{
}

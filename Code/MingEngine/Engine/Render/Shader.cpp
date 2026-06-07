#include "MingEngine/Engine/Render/Shader.hpp"

#include <d3d11.h>

Shader::Shader(const ShaderConfig& config) : m_config(config)
{
}

Shader::~Shader()
{
    if (m_inputLayout) { m_inputLayout->Release();  m_inputLayout = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release();  m_pixelShader = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
}

const std::string& Shader::GetName() const
{
    return m_config.m_name;
}


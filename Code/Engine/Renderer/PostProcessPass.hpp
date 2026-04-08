#pragma once

#include "Engine/Renderer/Shader.hpp"

#include <string>

class PostProcessPass
{
    enum class PostProcessInputSource
    {
        SCENE_COLOR,
        PREVIOUS,
        COUNT
    };

    enum class PostProcessOutputTarget
    {
        BACKBUFFER,
        INTERMEDIATE,
        COUNT
    };

public:
    PostProcessPass(
        std::string const& passName, 
        std::string const& postProcessShaderName);
    ~PostProcessPass();

public:
    bool m_isEnabled = true;

    std::string m_name;
	std::wstring m_wideName; // For use in debug annotations

    Shader* m_postProcessShader = nullptr;
    PostProcessInputSource m_inputSource = PostProcessInputSource::SCENE_COLOR;
    PostProcessOutputTarget m_outputTarget = PostProcessOutputTarget::BACKBUFFER;
};
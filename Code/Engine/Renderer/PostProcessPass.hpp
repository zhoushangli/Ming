#pragma once

#include "Engine/Renderer/Shader.hpp"
#include "Engine/Math/IntVec2.hpp"

#include <string>

class PostProcessPass
{
public:
	PostProcessPass(std::string const& passName, std::string const& postProcessShaderName);
	~PostProcessPass();

public:
	bool m_isEnabled = true;

	std::string m_name              = "Undefined";
	std::wstring m_wideName; // For use in debug annotations
	Shader*     m_postProcessShader = nullptr;
};
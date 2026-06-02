#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Shader.hpp"

#include <string>
#include <vector>

struct OutputTextureRef
{
public:
	OutputTextureRef(std::string const& name) : m_name(name) {};
	bool IsValid() const { return m_name != "Undefined"; }

public:
	std::string m_name = "Undefined";
};

struct InputTextureRef
{
public:
	InputTextureRef(std::string const& name, int slot) : m_name(name), m_slot(slot) {};
	bool IsValid() const { return m_name != "Undefined" && m_slot != -1; }

public:
	std::string m_name = "Undefined";
	int         m_slot = -1;
};

class PostProcessPass
{
public:
	PostProcessPass(std::string const& passName, std::string const& postProcessShaderName);
	~PostProcessPass();

	bool HasCustomOutput() const;
	bool HasCustomInputs() const;

public:
	bool m_isEnabled = true;

	std::string  m_name = "Undefined";
	std::wstring m_wideName; // For use in debug annotations
	Shader*      m_postProcessShader = nullptr;

	OutputTextureRef             m_customOutput = OutputTextureRef("Undefined");
	std::vector<InputTextureRef> m_customInputs;
};
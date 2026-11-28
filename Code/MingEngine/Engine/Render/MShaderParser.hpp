#pragma once

#include "MingEngine/Core/Object/Variant.hpp"

#include <string>
#include <vector>

struct MShaderUniform
{
	enum class Binding
	{
		ConstantBuffer,
		ModelColor,
		Texture
	};

	std::string m_name;
	Variant m_defaultValue;
	unsigned int m_offset = 0;
	unsigned int m_components = 1;
	Binding m_binding = Binding::ConstantBuffer;
	unsigned int m_textureSlot = 0;
};

struct MShaderParseResult
{
	std::string m_source;
	std::string m_uniformSource;
	std::vector<MShaderUniform> m_uniforms;
	bool m_hasVertex = false;
	bool m_hasFragment = false;
};

namespace MShaderParser
{
bool Parse(std::string const& source, std::string const& path, MShaderParseResult& result, std::string& error);
}

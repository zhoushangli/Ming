#pragma once

#include "MingEngine/Engine/Render/MShaderParser.hpp"
#include "MingEngine/Engine/Render/TextureBindingSlots.hpp"

#include <string>
#include <string_view>

namespace BuiltinShaders
{
inline std::vector<MShaderUniform> GetMaterialUniforms(bool canvas)
{
	MShaderUniform tint;
	tint.m_name = "tint";
	tint.m_defaultValue = Color::White;
	tint.m_binding = MShaderUniform::Binding::ModelColor;
	std::vector<MShaderUniform> result = { tint };
	if (!canvas)
	{
		char const* names[] = { "diffuse_texture", "normal_texture", "sge_texture" };
		for (unsigned int slot = 0; slot < SurfaceTextureSlot::Count; ++slot)
		{
			MShaderUniform texture;
			texture.m_name = names[slot];
			texture.m_defaultValue = Variant::FromRef(nullptr);
			texture.m_binding = MShaderUniform::Binding::Texture;
			texture.m_textureSlot = slot;
			result.push_back(texture);
		}
	}
	return result;
}

// Assemble common declarations, user callbacks and the Lit entry template with source diagnostics.
// e.g. BuildSurfaceShaderSource(userSource, "C:/Project/MoveUp.hlsl")
std::string BuildSurfaceShaderSource(std::string const& userSource, std::string const& sourcePath, std::string const& uniformSource = {});

// Store the built-in Lit source assembled with empty vertex and fragment callbacks.
// e.g. std::string_view source = BuiltinShaders::DefaultLit
extern std::string_view const DefaultLit;

// Store the built-in DefaultUI shader source with all includes expanded.
// e.g. std::string_view source = BuiltinShaders::DefaultUI
extern std::string_view const DefaultUI;

// Store the built-in DefaultUnlit shader source with all includes expanded.
// e.g. std::string_view source = BuiltinShaders::DefaultUnlit
extern std::string_view const DefaultUnlit;

// Store the built-in GizmosAxis shader source with all includes expanded.
// e.g. std::string_view source = BuiltinShaders::GizmosAxis
extern std::string_view const GizmosAxis;

// Store the built-in GizmosGrid shader source with all includes expanded.
// e.g. std::string_view source = BuiltinShaders::GizmosGrid
extern std::string_view const GizmosGrid;

// Store the built-in PostProcessCopy shader source with all includes expanded.
// e.g. std::string_view source = BuiltinShaders::PostProcessCopy
extern std::string_view const PostProcessCopy;

// Store the built-in TransformGizmosArc shader source with all includes expanded.
// e.g. std::string_view source = BuiltinShaders::TransformGizmosArc
extern std::string_view const TransformGizmosArc;
} // namespace BuiltinShaders

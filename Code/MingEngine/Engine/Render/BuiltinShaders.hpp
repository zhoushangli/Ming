#pragma once

#include <string_view>

namespace BuiltinShaders
{
	// Store the built-in DefaultLit shader source with all includes expanded.
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

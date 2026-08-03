#pragma once

#include <filesystem>

class CSharpScriptGenerator
{
public:
	bool GenerateCSharpBindings(std::filesystem::path const &outputDirectory);
};
#pragma once

#include <filesystem>

class CSharpScriptGenerator
{
public:
	bool GenerateCSharpBindings(std::filesystem::path const &outputDirectory);

private:
	bool GenerateNativeCalls(std::filesystem::path const &outputDirectory);
	bool GenerateClassBindings(std::filesystem::path const &outputDirectory);
};
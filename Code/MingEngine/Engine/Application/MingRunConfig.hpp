#pragma once

#include <filesystem>

enum class MingRunMode
{
	ProjectManager,
	Editor,
	Game
};

struct MingRunConfig
{
	MingRunMode           mode = MingRunMode::ProjectManager;
	std::filesystem::path projectPath;

	// Generate C# bindings as a command-line task without starting the application.
	// e.g. MingEngine_Debug.exe --generate-bindings <output directory>
	bool                  generateCSharpBindings = false;
	std::filesystem::path csharpBindingsOutputDirectory;
};

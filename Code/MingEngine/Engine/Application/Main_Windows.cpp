#include "MingEngine/Engine/Application/Main_Windows.hpp"

#include "MingEngine/Engine/Application/App.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

namespace
{
std::filesystem::path GetExecutableDirectory()
{
	std::wstring executablePath(32768, L'\0');

	DWORD const length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
	if (length == 0 || length >= executablePath.size())
	{
		return {};
	}

	executablePath.resize(length);
	return std::filesystem::path(executablePath).parent_path();
}

bool TryParseRunConfig(MingRunConfig& outConfig, int argc, wchar_t** argv)
{
	for (int i = 1; i < argc; ++i)
	{
		std::wstring const arg = argv[i];
		if (arg == L"--game")
		{
			outConfig.mode = MingRunMode::Game;
		}
		else if (arg == L"--editor")
		{
			outConfig.mode = MingRunMode::Editor;
		}
		else if (arg == L"--project" && i + 1 < argc)
		{
			outConfig.projectPath = argv[i + 1];
			++i;
		}
		else if (std::filesystem::path(arg).filename() == L"project.ming")
		{
			std::filesystem::path projectSettingsPath = arg;

			outConfig.mode        = MingRunMode::Editor;
			outConfig.projectPath = projectSettingsPath;
		}
		else if (arg == L"--generate-bindings")
		{
			std::filesystem::path const executableDirectory = GetExecutableDirectory();
			if (executableDirectory.empty())
			{
				return false;
			}

			std::filesystem::path const engineProjectDirectory = executableDirectory.parent_path();
			std::filesystem::path       csharpProjectDirectory;
			if (i + 1 < argc && argv[i + 1] != nullptr && argv[i + 1][0] != L'\0')
			{
				csharpProjectDirectory = argv[i + 1];
				if (csharpProjectDirectory.is_relative())
				{
					csharpProjectDirectory = engineProjectDirectory / csharpProjectDirectory;
				}
			}
			else
			{
				csharpProjectDirectory = engineProjectDirectory / "Code" / "CSharp" / "MingSharp";
			}

			outConfig.generateCSharpBindings = true;
			// lexically_normal is for safety
			// e.g. C:\Work\Ming\Run\..\Code\CSharp\MingSharp -> C:\Work\Ming\Code\CSharp\MingSharp
			outConfig.csharpBindingsOutputDirectory = csharpProjectDirectory.lexically_normal();
			return true;
		}
		else
		{
			return false;
		}
	}

	if (outConfig.projectPath.empty())
	{
		if (outConfig.mode == MingRunMode::Game)
		{
			return false;
		}

		outConfig.mode = MingRunMode::ProjectManager;
	}
	else
	{
		std::error_code error;
		outConfig.projectPath = std::filesystem::absolute(outConfig.projectPath, error);
		if (error)
		{
			return false;
		}

		if (outConfig.projectPath.filename() == L"project.ming")
		{
			outConfig.projectPath = outConfig.projectPath.parent_path();
		}

		if (!std::filesystem::is_regular_file(outConfig.projectPath / L"project.ming", error) || error)
		{
			return false;
		}

		if (outConfig.mode == MingRunMode::ProjectManager)
		{
			outConfig.mode = MingRunMode::Editor;
		}
	}

	return true;
}

int Main(int argc, wchar_t** wc_argv)
{
	wc_argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (wc_argv == nullptr)
	{
		return EXIT_FAILURE;
	}

	MingRunConfig config;
	bool          success = TryParseRunConfig(config, argc, wc_argv);
	LocalFree(wc_argv);

	if (!success)
	{
		MessageBoxW(
			nullptr,
			L"Invalid startup arguments or project path. Select a directory containing project.ming.",
			L"MingEngine",
			MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	return MingEngine::Run(config);
}
}

int WINAPI
WinMain([[maybe_unused]] HINSTANCE applicationInstanceHandle, HINSTANCE, [[maybe_unused]] LPSTR commandLineString, int)
{
	return Main(0, nullptr);
}

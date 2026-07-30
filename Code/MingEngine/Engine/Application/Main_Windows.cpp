#include "MingEngine/Engine/Application/Main_Windows.hpp"

#include "MingEngine/Engine/Application/App.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

namespace
{
	bool EndsWith(std::wstring const &str, std::wstring const &suffix)
	{
		if (str.length() < suffix.length())
		{
			return false;
		}

		return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
	}

	bool TryParseRunConfig(MingRunConfig &outConfig, int argc, wchar_t **argv)
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
			else if (EndsWith(arg, L"project.ming"))
			{
				std::filesystem::path projectSettingsPath = arg;

				outConfig.mode = MingRunMode::Editor;
				outConfig.projectPath = projectSettingsPath.parent_path();
				
				return true;
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	int Main(int argc, wchar_t **wc_argv)
	{
		wc_argv = CommandLineToArgvW(GetCommandLineW(), &argc);

		MingRunConfig config;
		bool success = TryParseRunConfig(config, argc, wc_argv);

		if (!success)
		{
			return EXIT_FAILURE;
		}

		return MingEngine::Run(config);
	}
}

int WINAPI WinMain([[maybe_unused]] HINSTANCE applicationInstanceHandle, HINSTANCE, [[maybe_unused]] LPSTR commandLineString, int)
{
	return Main(0, nullptr);
}

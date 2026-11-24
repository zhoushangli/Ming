#include "MingEngine/Editor/EditorCSharpBuildManager.hpp"
#include "MingEngine/Editor/EditorSceneManager.hpp"
#include "MingEngine/Editor/EditorRunManager.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <filesystem>
#include <cwchar>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetClassName

namespace
{
std::filesystem::path FindVSCode()
{
	std::vector<std::filesystem::path> candidates;
	for (auto const* name : { L"Code.exe", L"code.cmd" })
	{
		DWORD const length = SearchPathW(nullptr, name, nullptr, 0, nullptr, nullptr);
		if (length == 0)
		{
			continue;
		}

		std::wstring buffer(length, L'\0');
		DWORD const written = SearchPathW(nullptr, name, nullptr, length, buffer.data(), nullptr);
		if (written == 0 || written >= length)
		{
			continue;
		}

		buffer.resize(written);
		std::filesystem::path const found(buffer);
		candidates.push_back(found.extension() == L".cmd" ? found.parent_path().parent_path() / L"Code.exe" : found);
	}

	for (auto const* variable : { L"LOCALAPPDATA", L"ProgramFiles", L"ProgramFiles(x86)" })
	{
		DWORD const length = GetEnvironmentVariableW(variable, nullptr, 0);
		if (length == 0)
		{
			continue;
		}

		std::wstring buffer(length, L'\0');
		DWORD const written = GetEnvironmentVariableW(variable, buffer.data(), length);
		if (written == 0 || written >= length)
		{
			continue;
		}

		buffer.resize(written);
		std::filesystem::path directory(buffer);
		if (std::wstring(variable) == L"LOCALAPPDATA")
		{
			directory /= L"Programs";
		}

		candidates.push_back(directory / L"Microsoft VS Code" / L"Code.exe");
	}

	for (auto const& candidate : candidates)
	{
		std::error_code error;
		if (std::filesystem::is_regular_file(candidate, error))
		{
			return candidate;
		}
	}

	return {};
}
}

EditorCSharpBuildManager::EditorCSharpBuildManager(EditorNode& editor) : m_editor(editor)
{
}

bool EditorCSharpBuildManager::GenerateCSharpProject()
{
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject())
	{
		return false;
	}

	if (!g_engine->m_scriptSystem->EnsureProjectSolution())
	{
		m_editor.m_editorUI->Warning("C# Project", "Could not generate the C# project. See Output.");
		return false;
	}
	INFO_PRINT("C# project generated.");
	return true;
}

bool EditorCSharpBuildManager::OpenCSharpScript(VirtualPath const& path)
{
	auto fail = [this](std::string const& message)
	{
		ERR_PRINT(message);
		m_editor.m_editorUI->Warning("C# Editor", message);
		return false;
	};

	std::filesystem::path script;
	std::filesystem::path project;
	std::error_code error;
	if (!path.HasExtension(".cs") || !g_engine->m_fileSystem->TryGetPhysicalPath(path, script)
		|| !std::filesystem::is_regular_file(script, error)
		|| !g_engine->m_fileSystem->TryGetPhysicalPath(VirtualPath("res://"), project))
	{
		return fail("Could not find the C# script in the current project.");
	}

	std::filesystem::path const executable = FindVSCode();
	if (executable.empty())
	{
		return fail("Could not find Visual Studio Code. Install it in the default location or add its bin folder to PATH.");
	}

	if (!g_engine->m_scriptSystem->EnsureProjectSolution())
	{
		return fail("Could not prepare the C# project. See Output.");
	}

	// Start VS Code as an editor even when Ming was launched from an Electron tool.
	// e.g. Remove ELECTRON_RUN_AS_NODE=1 from the child environment only.
	wchar_t* inherited = GetEnvironmentStringsW();
	if (inherited == nullptr)
	{
		return fail("Could not read the environment for Visual Studio Code.");
	}

	std::wstring environment;
	for (wchar_t const* entry = inherited; *entry != L'\0'; entry += std::wcslen(entry) + 1)
	{
		wchar_t const* key = L"ELECTRON_RUN_AS_NODE=";
		if (_wcsnicmp(entry, key, std::wcslen(key)) != 0)
		{
			environment.append(entry, std::wcslen(entry) + 1);
		}
	}

	environment.push_back(L'\0');
	FreeEnvironmentStringsW(inherited);

	std::wstring command = L"\"" + executable.wstring() + L"\" \"" + (project / L".").wstring()
		+ L"\" \"" + script.wstring() + L"\"";
	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
		environment.data(), project.c_str(), &startup, &process))
	{
		return fail("Could not start Visual Studio Code. Windows error: " + std::to_string(GetLastError()));
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}

bool EditorCSharpBuildManager::BuildCSharpProject()
{
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject() || (g_app != nullptr && g_app->IsGameRunning()))
	{
		return false;
	}

	return BeginCSharpBuild();
}

bool EditorCSharpBuildManager::BeginCSharpBuild()
{
	m_editor.m_editorUI->FinishNativeEditing();
	m_editor.m_editorUI->GetPanel<ViewportPanel>().CancelNavigation();
	m_editor.m_editorGizmos->CancelDrag();
	auto* viewport = m_editor.m_editorUI->GetRootControl()->GetViewport();
	viewport->CancelGuiInteraction(viewport);

	m_buildExitCode = -1;
	if (!g_engine->m_scriptSystem->EnsureProjectSolution() || !g_engine->m_scriptSystem->BeginProjectBuild())
	{
		g_engine->m_scriptSystem->PollProjectBuild(m_buildExitCode);
		g_engine->m_scriptSystem->CloseProjectBuild();
		m_editor.m_sceneManager->CancelPendingSceneAction();
		m_editor.m_editorUI->Warning("C# Build", "Build failed. See Output.");
		return false;
	}

	m_buildStarted = std::chrono::steady_clock::now();
	m_buildInProgress = true;

	m_editor.m_editorUI->SetStatusMessage("Checking and building C# project...");
	return true;
}

void EditorCSharpBuildManager::UpdateCSharpBuild()
{
	if (!m_buildInProgress)
	{
		return;
	}

	auto* scripts = g_engine->m_scriptSystem;
	ScriptSystem::BuildState const state = scripts->PollProjectBuild(m_buildExitCode);
	if (state == ScriptSystem::BuildState::Running)
	{
		if (std::chrono::steady_clock::now() - m_buildStarted >= std::chrono::milliseconds(250))
		{
			m_editor.m_editorUI->ShowBuildProgress("Checking and building C# project...");
		}
		return;
	}

	m_buildInProgress = false;

	scripts->CloseProjectBuild();
	if (state != ScriptSystem::BuildState::Succeeded)
	{
		m_editor.m_sceneManager->CancelPendingSceneAction();
		m_editor.m_editorUI->HideBuildProgress();
		m_editor.m_editorUI->SetStatusMessage("C# build failed.");
		m_editor.m_editorUI->Warning("C# Build", "Build failed. See Output.");
		return;
	}

	INFO_PRINT("C# build completed.");
	m_editor.m_runManager->BuildFinished(true);
	m_editor.m_editorUI->HideBuildProgress();
	m_editor.m_editorUI->SetStatusMessage("C# build completed.");
}

bool EditorCSharpBuildManager::ReloadCSharpAssembly()
{
	if (m_editor.IsBuildingCSharpProject() || m_editor.IsPreparingPlay() || m_editor.IsCheckingProject())
	{
		return false;
	}

	if (!g_engine->m_scriptSystem->ReloadProjectAssembly())
	{
		m_editor.m_editorUI->Warning("C# Reload", "Reload failed. See Output.");
		return false;
	}
	INFO_PRINT("C# assembly reloaded.");
	return true;
}

EditorCSharpBuildManager::~EditorCSharpBuildManager()
{
	if (g_engine != nullptr && g_engine->m_scriptSystem != nullptr)
	{
		g_engine->m_scriptSystem->CloseProjectBuild();
	}
}

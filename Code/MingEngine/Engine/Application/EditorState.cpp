#include "MingEngine/Engine/Application/EditorState.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/ProjectManagerState.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Scene/SceneSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>

bool EditorState::Enter(App& app)
{
	glfwSetWindowSizeLimits(g_engine->m_windowSystem->GetGLFWWindow(), 900, 540, GLFW_DONT_CARE, GLFW_DONT_CARE);
	m_app = &app;

	auto editorNode = new EditorNode(EditorThemeManager::LoadScale());
	editorNode->SetName("EditorNode");
	g_engine->m_sceneSystem->GetSceneTree()->GetRoot()->AddNode(editorNode);
	ProjectManagerState::RememberProject(app.GetRunConfig().projectPath);
	VirtualPath const& startScenePath = ProjectSettings::Get()->m_startScenePath;
	if (startScenePath.IsValid())
	{
		if (!editorNode->LoadScene(startScenePath))
		{
			editorNode->m_editorUI->Warning("Open Scene Failed", startScenePath.GetString());
		}
	}

	return true;
}

void EditorState::Update([[maybe_unused]] App& app, [[maybe_unused]] float deltaSeconds)
{
	UpdateGameProcess();

	if (!g_engine->m_inputSystem->IsWindowFocused())
	{
		g_engine->m_inputSystem->SetCursorMode(CursorMode::POINTER);
		g_engine->m_inputSystem->ClearCursorDelta();
	}
}

void EditorState::Exit([[maybe_unused]] App& app)
{
	StopGame();
	if (m_gameProcess != nullptr)
	{
		if (WaitForSingleObject(m_gameProcess, 2000) == WAIT_TIMEOUT)
		{
			TerminateProcess(m_gameProcess, 0);
			WaitForSingleObject(m_gameProcess, 1000);
		}
		CloseHandle(m_gameProcess);
		m_gameProcess = nullptr;
	}
	ReadGameOutput();
	if (m_gameOutputRead != nullptr)
	{
		CloseHandle(m_gameOutputRead);
		m_gameOutputRead = nullptr;
	}

	m_gameProcessID    = 0;
	m_gameStopDeadline = 0;
	m_gameOutput.clear();
	m_app = nullptr;
}

bool EditorState::CanQuit([[maybe_unused]] App& app)
{
	if (EditorNode* editor = EditorNode::Get(); editor != nullptr)
	{
		editor->m_editorUI->FinishNativeEditing();
	}

	if (EditorNode* editor = EditorNode::Get(); editor != nullptr && editor->IsSceneDirty())
	{
		glfwSetWindowShouldClose(g_engine->m_windowSystem->GetGLFWWindow(), GLFW_FALSE);
		editor->RequestQuit();
		return false;
	}

	StopGame();
	return true;
}

bool EditorState::LaunchGame()
{
	UpdateGameProcess();
	if (IsGameRunning())
	{
		return false;
	}
	// 1) Get the current executable path
	std::wstring executablePath(32768, L'\0');

	DWORD const length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));

	if (length == 0 || length >= executablePath.size())
	{
		ERR_PRINT("Failed to get the executable path.\n");
		return false;
	}

	executablePath.resize(length);

	// 2) Resolve the current project directory
	if (m_app->GetRunConfig().projectPath.empty())
	{
		ERR_PRINT("Cannot launch game: project path is empty.\n");
		return false;
	}

	std::error_code             error;
	std::filesystem::path const projectDirectory = std::filesystem::absolute(m_app->GetRunConfig().projectPath, error);

	if (error)
	{
		ERR_PRINT("Failed to resolve the project directory.\n");
		return false;
	}

	std::wstring const projectArgument = (projectDirectory / L".").wstring();

	// 3) Build the game-mode command line
	std::wstring commandLine = L"\"" + executablePath + L"\" --game --project \"" + projectArgument + L"\"";

	STARTUPINFOW startupInfo{};
	startupInfo.cb = sizeof(startupInfo);
	SECURITY_ATTRIBUTES security{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
	HANDLE              outputRead  = nullptr;
	HANDLE              outputWrite = nullptr;
	if (!CreatePipe(&outputRead, &outputWrite, &security, 0))
	{
		ERR_PRINT("Could not create the game output pipe.");
		return false;
	}

	SetHandleInformation(outputRead, HANDLE_FLAG_INHERIT, 0);
	HANDLE input =
		CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr);
	startupInfo.dwFlags    = STARTF_USESTDHANDLES;
	startupInfo.hStdOutput = outputWrite;
	startupInfo.hStdError  = outputWrite;
	startupInfo.hStdInput  = input;

	PROCESS_INFORMATION processInfo{};

	// 4) Start the game process
	if (!CreateProcessW(
			executablePath.c_str(),
			commandLine.data(),
			nullptr,
			nullptr,
			TRUE,
			CREATE_NO_WINDOW,
			nullptr,
			nullptr,
			&startupInfo,
			&processInfo))
	{
		DWORD const       errorCode = GetLastError();
		std::string const message   = "Failed to launch game. Windows error: " + std::to_string(errorCode) + "\n";

		ERR_PRINT(message.c_str());
		CloseHandle(outputRead);
		CloseHandle(outputWrite);
		CloseHandle(input);
		return false;
	}
	CloseHandle(outputWrite);
	CloseHandle(input);
	m_gameOutputRead = outputRead;
	m_gameOutput.clear();

	// 5) Release the launch-only process handles
	CloseHandle(processInfo.hThread);
	m_gameProcess      = processInfo.hProcess;
	m_gameProcessID    = processInfo.dwProcessId;
	m_gameStopDeadline = 0;
	INFO_PRINT("Game process started.");
	return true;
}

void EditorState::StopGame()
{
	if (m_gameProcess == nullptr || m_gameStopDeadline != 0)
	{
		return;
	}
	EnumWindows(
		[](HWND window, LPARAM parameter) -> BOOL
		{
			DWORD process = 0;
			GetWindowThreadProcessId(window, &process);
			if (process == static_cast<DWORD>(parameter))
			{
				PostMessageW(window, WM_CLOSE, 0, 0);
			}
			return TRUE;
		},
		static_cast<LPARAM>(m_gameProcessID));
	m_gameStopDeadline = GetTickCount64() + 2000;
}

void EditorState::UpdateGameProcess()
{
	ReadGameOutput();
	if (m_gameProcess == nullptr)
	{
		return;
	}
	if (m_gameStopDeadline != 0 && GetTickCount64() >= m_gameStopDeadline
		&& WaitForSingleObject(m_gameProcess, 0) == WAIT_TIMEOUT)
	{
		TerminateProcess(m_gameProcess, 0);
	}
	if (WaitForSingleObject(m_gameProcess, 0) == WAIT_OBJECT_0)
	{
		ReadGameOutput();
		if (m_gameOutputRead != nullptr)
		{
			CloseHandle(m_gameOutputRead);
			m_gameOutputRead = nullptr;
		}

		if (!m_gameOutput.empty())
		{
			INFO_PRINT("[Game] " + m_gameOutput);
			m_gameOutput.clear();
		}

		DWORD code = 0;
		GetExitCodeProcess(m_gameProcess, &code);
		CloseHandle(m_gameProcess);
		m_gameProcess      = nullptr;
		m_gameProcessID    = 0;
		m_gameStopDeadline = 0;
		INFO_PRINT("Game process exited with code " + std::to_string(code));
	}
}

void EditorState::ReadGameOutput()
{
	if (m_gameOutputRead == nullptr)
	{
		return;
	}

	DWORD available = 0;
	char  buffer[4096];
	while (PeekNamedPipe(m_gameOutputRead, nullptr, 0, nullptr, &available, nullptr) && available > 0)
	{
		DWORD read = 0;
		if (!ReadFile(
				m_gameOutputRead,
				buffer,
				(std::min)(available, static_cast<DWORD>(sizeof(buffer))),
				&read,
				nullptr)
			|| read == 0)
		{
			break;
		}

		m_gameOutput.append(buffer, read);
		size_t end = 0;
		while ((end = m_gameOutput.find('\n')) != std::string::npos)
		{
			std::string line = m_gameOutput.substr(0, end);
			m_gameOutput.erase(0, end + 1);
			if (line.find("[Error]") != std::string::npos)
			{
				ERR_PRINT("[Game] " + line);
			}
			else
			{
				INFO_PRINT("[Game] " + line);
			}
		}
	}
}

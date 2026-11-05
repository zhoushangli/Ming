#include "MingEngine/Engine/Application/App.hpp"

#include "MingEngine/Core/Clock.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Memory.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Script/CSharpScript.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

#include "ThirdParty/GLFW/glfw3.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <fstream>
#include <cwctype>
#include "ThirdParty/nlohmann/json.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include <memory>
#include <string>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")

App* g_app = nullptr;

App::App(MingRunConfig const& config) : m_runConfig(config)
{
	EngineConfig engineConfig;
	engineConfig.m_windowConfig.m_clientAspect     = 16.f / 9.f;
	engineConfig.m_windowConfig.m_appName          = "MingEngine";
	engineConfig.m_fileSystemConfig.m_resourceRoot = config.projectPath;
	if (config.mode == MingRunMode::ProjectManager)
	{
		engineConfig.m_windowConfig.m_appName = "MingEngine - Project Manager";
	}

	g_engine = new Engine(engineConfig);
	g_engine->SetEditorMode(config.mode == MingRunMode::Editor);
}

App::~App()
{
	ShutdownScene();

	delete g_engine;
	g_engine = nullptr;
}

void App::Startup()
{
	// Initialize the UI thread's COM apartment before audio initializes its backend.
	// e.g. Browse uses an STA file dialog even when miniaudio also uses COM.
	HRESULT const initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	m_comInitialized = SUCCEEDED(initialized);
	if (!m_comInitialized)
	{
		WARN_PRINT(Stringf("Could not initialize the UI COM apartment (HRESULT 0x%08X).", initialized));
	}

	ClassDatabase::Startup();

	RegisterAllTypes();

	g_engine->Startup();
	if (m_runConfig.mode != MingRunMode::ProjectManager)
	{
		std::string error;
		if (!ProjectSettings::Load(m_runConfig.projectPath, error))
		{
			ERR_PRINT(error);
			if (m_runConfig.mode == MingRunMode::Editor)
			{
				MessageBoxA(nullptr, error.c_str(), "MingEngine - Project Error", MB_OK | MB_ICONERROR);
			}
			m_exitCode   = 1;
			m_shouldQuit = true;
			return;
		}
	}
	if (m_runConfig.mode == MingRunMode::Editor)
	{
		StartupEditor();
	}
	else if (m_runConfig.mode == MingRunMode::Game)
	{
		StartupGame();
	}
	else
	{
		GLFWwindow* window = g_engine->m_windowSystem->GetGLFWWindow();
		glfwRestoreWindow(window);
		glfwSetWindowSizeLimits(window, 900, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
		glfwSetWindowSize(window, 1000, 700);
	}

	m_quitEventID = RegisterEvent("Quit", App::OnQuit);
}

void App::Shutdown()
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

	ShutdownScene();

	if (g_engine != nullptr && g_engine->m_eventSystem != nullptr)
	{
		UnregisterEvent("Quit", m_quitEventID);
		m_quitEventID = 0;
	}

	ProjectSettings::Reset();
	ThemeDatabase::Shutdown();

	ResourceLoader::Shutdown();

	EditorIcons::Shutdown();

	g_engine->Shutdown();

	ClassDatabase::Shutdown();

	if (m_comInitialized)
	{
		CoUninitialize();
		m_comInitialized = false;
	}
}

void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::Update(float deltaSeconds)
{
	PollGame();
	if (m_runConfig.mode == MingRunMode::ProjectManager)
	{
		DrawProjectManager();
		m_sceneTree->UpdateScene(deltaSeconds);
		return;
	}

	GLFWwindow* window   = g_engine->m_windowSystem->GetGLFWWindow();
	bool const  hasFocus = window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED);

	if (!hasFocus)
	{
		g_engine->m_inputSystem->SetCursorMode(CursorMode::POINTER);
		if (!hasFocus)
		{
			g_engine->m_inputSystem->ClearCursorDelta();
		}
	}

	float const sceneDeltaSeconds = m_clock != nullptr ? static_cast<float>(m_clock->GetDeltaSeconds()) : deltaSeconds;
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->UpdateScene(sceneDeltaSeconds);
	}
}

void App::Render() const
{
	if (g_engine != nullptr && g_engine->m_renderServer != nullptr)
	{
		g_engine->m_renderServer->Render();
	}
}

void App::BeginFrame()
{
	g_engine->BeginFrame();
}

void App::EndFrame()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->FlushPendingNode();
	}

	g_engine->EndFrame();

	if (m_shouldRestart)
	{
		RestartImmediately();
		m_shouldRestart = false;
	}
}

void App::RunFrame()
{
	BeginFrame();

	Clock::TickSystemClock();
	float const deltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

	Update(deltaSeconds);
	Render();
	EndFrame();
}

void App::Restart()
{
	m_shouldRestart = true;
}

void App::Quit()
{
	if (EditorNode* editor = EditorNode::Get(); editor != nullptr)
	{
		editor->m_editorUI->FinishNativeEditing();
	}

	if (EditorNode* editor = EditorNode::Get(); editor != nullptr && editor->IsSceneDirty())
	{
		glfwSetWindowShouldClose(g_engine->m_windowSystem->GetGLFWWindow(), GLFW_FALSE);
		editor->RequestQuit();
		return;
	}

	StopGame();
	m_shouldQuit = true;
}

bool App::LaunchGame()
{
	PollGame();
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
	if (m_runConfig.projectPath.empty())
	{
		ERR_PRINT("Cannot launch game: project path is empty.\n");
		return false;
	}

	std::error_code             error;
	std::filesystem::path const projectDirectory = std::filesystem::absolute(m_runConfig.projectPath, error);

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
	HANDLE outputRead = nullptr;
	HANDLE outputWrite = nullptr;
	if (!CreatePipe(&outputRead, &outputWrite, &security, 0))
	{
		ERR_PRINT("Could not create the game output pipe.");
		return false;
	}

	SetHandleInformation(outputRead, HANDLE_FLAG_INHERIT, 0);
	HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr);
	startupInfo.dwFlags = STARTF_USESTDHANDLES;
	startupInfo.hStdOutput = outputWrite;
	startupInfo.hStdError = outputWrite;
	startupInfo.hStdInput = input;

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

bool App::OnQuit([[maybe_unused]] EventArgs& args)
{
	if (g_app != nullptr)
	{
		g_app->Quit();
	}

	return true;
}

void App::RestartImmediately()
{
	ShutdownScene();

	if (m_runConfig.mode == MingRunMode::Editor)
	{
		StartupEditor();
	}
	else if (m_runConfig.mode == MingRunMode::Game)
	{
		StartupGame();
	}
}

namespace
{
std::filesystem::path SelectProjectFolder(HWND owner, std::string& error)
{
	error.clear();
	HRESULT const initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(initialized))
	{
		error = Stringf("Could not initialize the folder dialog (HRESULT 0x%08X).", initialized);
		return {};
	}

	IFileOpenDialog*      dialog      = nullptr;
	std::filesystem::path result;
	HRESULT status = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (SUCCEEDED(status))
	{
		status = dialog->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
		if (SUCCEEDED(status))
		{
			status = dialog->Show(owner);
		}

		if (SUCCEEDED(status))
		{
			IShellItem* item = nullptr;
			status = dialog->GetResult(&item);
			if (SUCCEEDED(status))
			{
				PWSTR path = nullptr;
				status = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
				if (SUCCEEDED(status))
				{
					result = path;
					CoTaskMemFree(path);
				}
				item->Release();
			}
		}
		dialog->Release();
	}

	CoUninitialize();
	if (FAILED(status) && status != HRESULT_FROM_WIN32(ERROR_CANCELLED))
	{
		error = Stringf("Could not select a project folder (HRESULT 0x%08X).", status);
	}

	return result;
}

std::wstring ProjectKey(std::filesystem::path const& directory)
{
	std::error_code error;
	auto            normalized = std::filesystem::weakly_canonical(directory, error);
	std::wstring    key        = (error ? directory.lexically_normal() : normalized).wstring();
	std::transform(
		key.begin(),
		key.end(),
		key.begin(),
		[](wchar_t value)
		{
			return static_cast<wchar_t>(std::towlower(value));
		});
	return key;
}
}

void App::DrawProjectManager()
{
	if (m_sceneTree == nullptr)
	{
		m_sceneTree          = MemNew<SceneTree>();
		auto* background     = MemNew<ColorRect>();
		m_projectManagerRoot = background;
		background->SetName("ProjectManager");
		background->SetClipContents(true);
		m_sceneTree->GetRoot()->AddNode(background);
		Ref<Theme> theme =
			EditorThemeManager::CreateScaledTheme(EditorThemeManager::CreateTheme(), EditorThemeManager::LoadScale());
		ThemeDatabase::SetThemeContext(background, { theme });
		background->SetColor(theme->GetColor("background", "Editor"));
		g_engine->m_windowSystem->SetTitleBarColors(
			theme->GetColor("background", "Editor"),
			theme->GetColor("font_color", "Control"));
		auto*       content = MemNew<VBoxContainer>();
		float const padding = static_cast<float>(background->GetThemeConstant("content_padding", "Editor"));
		content->SetPosition(Vector2(padding, padding));
		content->AddThemeChangedCallback(
			[control = content]()
			{
				control->SetSeparation(static_cast<float>(control->GetThemeConstant("content_padding", "Editor")));
			});
		background->AddNode(content);
		EditorPopupUtils::AddLabel(content, "Ming - Project Manager", "EditorInspectorTitle");
		auto* tools = MemNew<HBoxContainer>();
		tools->AddThemeChangedCallback(
			[control = tools]()
			{
				control->SetSeparation(static_cast<float>(control->GetThemeConstant("content_padding", "Editor")));
			});
		content->AddNode(tools);
		EditorPopupUtils::AddButton(
			tools,
			"New Project...",
			[this]()
			{
				NewProject();
			});
		EditorPopupUtils::AddButton(
			tools,
			"Import / Open...",
			[this]()
			{
				OpenProject();
			});
		m_projectSearch = MemNew<LineEdit>();
		m_projectSearch->SetPlaceholder("Filter Projects");
		m_projectSearch->SetMinimumCharacterWidth(1);
		m_projectSearch->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		tools->AddNode(m_projectSearch);
		m_projectSearch->SetTextChangedCallback(
			[this](String const&)
			{
				m_recentDirty = true;
			});
		auto* scroll = MemNew<ScrollContainer>();
		scroll->SetVerticalSizeFlags(Control::Size_ExpandFill);
		content->AddNode(scroll);
		m_recentTree = MemNew<Tree>();
		m_recentTree->SetClipText(true);
		scroll->AddNode(m_recentTree);
		m_recentTree->SetActivatedCallback(
			[this](uint64_t key)
			{
				if (key > 0 && key <= m_recentAvailable.size() && m_recentAvailable[key - 1])
				{
					OpenProject(m_recentProjects[key - 1]);
				}
			});
		m_projectManagerErrorLabel = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		auto* actions              = EditorPopupUtils::AddActions(content);
		m_editProjectButton        = EditorPopupUtils::AddButton(
			actions,
			"Edit",
			[this]()
			{
				uint64_t const key = m_recentTree->GetSelected();
				if (key > 0 && key <= m_recentAvailable.size() && m_recentAvailable[key - 1])
				{
					OpenProject(m_recentProjects[key - 1]);
				}
			});
		m_removeProjectButton = EditorPopupUtils::AddButton(
			actions,
			"Remove",
			[this]()
			{
				uint64_t const key = m_recentTree->GetSelected();
				if (key > 0 && key <= m_recentProjects.size())
				{
					m_recentProjects.erase(m_recentProjects.begin() + key - 1);
					if (!SaveRecentProjects(m_recentProjects))
					{
						m_projectManagerError = "Could not save recent projects.";
					}
					m_recentDirty = true;
				}
			});
		m_recentProjects = LoadRecentProjects();
	}

	Vector2 const size(static_cast<Viewport*>(m_sceneTree->GetRoot())->GetOutputResolution());
	m_projectManagerRoot->UpdateSize(size);
	auto*         content = static_cast<Control*>(m_projectManagerRoot->GetChildren().front());
	Vector2 const padding = content->GetLocalPosition();
	content->UpdateSize(Vector2(std::max(1.f, size.x - padding.x * 2.f), std::max(1.f, size.y - padding.y * 2.f)));
	if (m_recentDirty)
	{
		RefreshRecentProjects();
	}
	uint64_t const selected     = m_recentTree->GetSelected();
	bool const     hasSelection = selected > 0 && selected <= m_recentAvailable.size();
	m_editProjectButton->SetDisabled(!hasSelection || !m_recentAvailable[selected - 1]);
	m_removeProjectButton->SetDisabled(!hasSelection);
	m_projectManagerErrorLabel->SetText(m_projectManagerError);
	if (m_createProjectPopup != nullptr && m_createProjectPopup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_createProjectPopup,
			Vector2(
				static_cast<float>(content->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(content->GetThemeConstant("create_popup_height", "Editor"))));
	}
}

void App::RefreshRecentProjects()
{
	m_recentAvailable.clear();
	std::vector<Tree::Item> items;
	for (size_t i = 0; i < m_recentProjects.size(); ++i)
	{
		std::string error;
		bool const  valid = ProjectSettings::Read(m_recentProjects[i], error).IsValid();
		m_recentAvailable.push_back(valid);
		std::string const text   = m_recentProjects[i].filename().u8string() + "   " + m_recentProjects[i].u8string()
								   + (valid ? "" : "  [Unavailable]");
		std::wstring      search = m_projectSearch->GetText().IsEmpty()
									   ? std::wstring()
									   : std::filesystem::u8path(m_projectSearch->GetText().ToUtf8()).wstring();
		std::transform(
			search.begin(),
			search.end(),
			search.begin(),
			[](wchar_t c)
			{
				return static_cast<wchar_t>(std::towlower(c));
			});
		items.push_back(
			{ i + 1,
			  0,
			  text,
			  search.empty() || ProjectKey(m_recentProjects[i]).find(search) != std::wstring::npos,
			  false,
			  m_projectManagerRoot->GetThemeIcon("Folder", "EditorIcons") });
		items.back().m_dimmed = !valid;
	}
	m_recentTree->SetItems(std::move(items));
	m_recentDirty = false;
}

void App::NewProject()
{
	if (m_createProjectPopup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_createProjectPopup   = EditorPopupUtils::Create(m_projectManagerRoot, "Create Empty Project", content);
		EditorPopupUtils::AddLabel(content, "Project Name (folder name)");
		m_projectName = MemNew<LineEdit>();
		m_projectName->SetMinimumCharacterWidth(1);
		content->AddNode(m_projectName);
		EditorPopupUtils::AddLabel(content, "Parent Directory");
		m_projectParent = MemNew<LineEdit>();
		m_projectParent->SetMinimumCharacterWidth(1);
		content->AddNode(m_projectParent);
		EditorPopupUtils::AddButton(
			content,
			"Browse...",
			[this]()
			{
				std::string error;
				auto directory = SelectProjectFolder(static_cast<HWND>(g_engine->m_windowSystem->GetHwnd()), error);

				// Discard input collected while the native modal dialog owns the window.
				// e.g. Its queued focus-loss event must not dismiss Create Empty Project after Browse returns.
				g_engine->m_inputSystem->GetInputEvents();
				g_engine->m_inputSystem->ClearAllInputStates();
				m_createProjectError->SetText(error);
				if (!directory.empty())
				{
					m_projectParent->SetText(directory.u8string());
				}

				m_projectParent->GrabFocus();
			});
		m_createProjectError = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		auto* actions        = EditorPopupUtils::AddActions(content);
		EditorPopupUtils::AddButton(
			actions,
			"Create & Edit",
			[this]()
			{
				std::string const name = m_projectName->GetText().ToUtf8();
				if (name.empty() || name == "." || name == ".."
					|| name.find_first_of("<>:\"/\\|?*") != std::string::npos || name.front() == ' '
					|| name.back() == ' ' || name.back() == '.')
				{
					m_createProjectError->SetText("Enter a valid project folder name.");
					return;
				}
				auto directory =
					std::filesystem::u8path(m_projectParent->GetText().ToUtf8()) / std::filesystem::u8path(name);
				std::string error;
				if (!CreateEmptyProject(directory, error))
				{
					m_createProjectError->SetText(error);
					return;
				}
				m_createProjectPopup->Close();
				OpenProject(directory);
			});
		EditorPopupUtils::AddButton(
			actions,
			"Cancel",
			[this]()
			{
				m_createProjectPopup->Close();
			});
	}
	m_createProjectError->SetText("");
	EditorPopupUtils::Open(
		m_createProjectPopup,
		m_projectManagerRoot,
		Vector2(
			static_cast<float>(m_projectManagerRoot->GetThemeConstant("project_popup_width", "Editor")),
			static_cast<float>(m_projectManagerRoot->GetThemeConstant("create_popup_height", "Editor"))));
	m_projectName->GrabFocus();
}

bool App::CreateEmptyProject(std::filesystem::path const& directory, std::string& error)
{
	error.clear();
	std::error_code code;
	if (!directory.is_absolute() || directory.filename().empty())
	{
		error = "Choose an absolute project directory.";
		return false;
	}
	bool const existed = std::filesystem::exists(directory, code);
	if (code
		|| (existed
			&& (!std::filesystem::is_directory(directory, code) || !std::filesystem::is_empty(directory, code))))
	{
		error = "The project directory must be empty.";
		return false;
	}
	if (!existed && !std::filesystem::create_directory(directory, code))
	{
		error = "Could not create the project directory.";
		return false;
	}
	HANDLE file = CreateFileW(
		(directory / L"project.ming").c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	bool success = false;
	if (file != INVALID_HANDLE_VALUE)
	{
		std::string const text    = R"({"type":"ProjectSettings","version":1,"start_scene_path":""})";
		DWORD             written = 0;
		success =
			WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) && written == text.size();
		CloseHandle(file);
		if (!success)
		{
			std::filesystem::remove(directory / L"project.ming", code);
		}
	}
	if (!success)
	{
		error = "Could not write project.ming.";
		if (!existed && std::filesystem::is_empty(directory, code))
		{
			std::filesystem::remove(directory, code);
		}
	}
	return success;
}

std::vector<std::filesystem::path> App::LoadRecentProjects()
{
	std::vector<std::filesystem::path> result;
	std::ifstream                      input(EditorThemeManager::GetSettingsDirectory() / L"RecentProjects.json");
	if (!input)
	{
		return result;
	}
	auto paths = nlohmann::json::parse(input, nullptr, false);
	if (paths.is_array())
	{
		for (auto const& path : paths)
		{
			if (path.is_string())
			{
				auto directory = std::filesystem::u8path(path.get<std::string>());
				if (directory.is_absolute()
					&& std::none_of(
						result.begin(),
						result.end(),
						[&](auto const& other)
						{
							return ProjectKey(other) == ProjectKey(directory);
						}))
				{
					result.push_back(directory);
				}
			}
			if (result.size() == 10)
			{
				break;
			}
		}
	}
	return result;
}

bool App::SaveRecentProjects(std::vector<std::filesystem::path> const& projects)
{
	std::error_code error;
	std::filesystem::create_directories(EditorThemeManager::GetSettingsDirectory(), error);
	if (error)
	{
		return false;
	}
	auto paths = nlohmann::json::array();
	for (auto const& project : projects)
	{
		paths.push_back(project.u8string());
		if (paths.size() == 10)
		{
			break;
		}
	}
	std::ofstream output(EditorThemeManager::GetSettingsDirectory() / L"RecentProjects.json");
	output << paths.dump(2);
	output.flush();
	return output.good();
}

void App::RememberProject(std::filesystem::path const& directory)
{
	auto               projects = LoadRecentProjects();
	std::wstring const key      = ProjectKey(directory);
	projects.erase(
		std::remove_if(
			projects.begin(),
			projects.end(),
			[&](auto const& other)
			{
				return ProjectKey(other) == key;
			}),
		projects.end());
	projects.insert(projects.begin(), directory);
	if (!SaveRecentProjects(projects))
	{
		WARN_PRINT("Could not save recent projects.");
	}
}

void App::OpenProject()
{
	m_projectManagerError.clear();
	std::wstring  selectedPath(32768, L'\0');
	OPENFILENAMEW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner   = static_cast<HWND>(g_engine->m_windowSystem->GetHwnd());
	dialog.lpstrFilter = L"Ming project (project.ming)\0*.ming\0\0";
	dialog.lpstrFile   = selectedPath.data();
	dialog.nMaxFile    = static_cast<DWORD>(selectedPath.size());
	dialog.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
	if (GetOpenFileNameW(&dialog))
	{
		std::filesystem::path settings(selectedPath.c_str());
		if (settings.filename() == L"project.ming")
		{
			OpenProject(settings.parent_path());
		}
		else
		{
			m_projectManagerError = "Select project.ming.";
		}
	}
	else if (CommDlgExtendedError() != 0)
	{
		m_projectManagerError = "Could not open the project file dialog.";
	}
}

bool App::OpenProject(std::filesystem::path const& directory)
{
	if (!ProjectSettings::Read(directory, m_projectManagerError).IsValid())
	{
		return false;
	}
	std::wstring executable(32768, L'\0');
	DWORD const  length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
	if (length == 0 || length >= executable.size())
	{
		m_projectManagerError = "Could not find the engine executable.";
		return false;
	}
	executable.resize(length);
	std::wstring command = L"\"" + executable + L"\" --editor --project \"" + (directory / L".").wstring() + L"\"";
	std::wstring working = std::filesystem::path(executable).parent_path().wstring();
	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	if (!CreateProcessW(
			executable.c_str(),
			command.data(),
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,
			working.c_str(),
			&startup,
			&process))
	{
		m_projectManagerError = "Could not launch the editor. Windows error: " + std::to_string(GetLastError());
		return false;
	}
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	RememberProject(directory);
	Quit();
	return true;
}

void App::StartupEditor()
{
	glfwSetWindowSizeLimits(g_engine->m_windowSystem->GetGLFWWindow(), 900, 540, GLFW_DONT_CARE, GLFW_DONT_CARE);
	m_clock     = new Clock();
	m_sceneTree = MemNew<SceneTree>();

	auto editorNode = new EditorNode(EditorThemeManager::LoadScale());
	editorNode->SetName("EditorNode");
	m_sceneTree->GetRoot()->AddNode(editorNode);
	RememberProject(m_runConfig.projectPath);
	VirtualPath const& startScenePath = ProjectSettings::Get()->m_startScenePath;
	if (startScenePath.IsValid())
	{
		if (!editorNode->LoadScene(startScenePath))
		{
			editorNode->m_editorUI->Warning("Open Scene Failed", startScenePath.GetString());
		}
	}
}

void App::StartupGame()
{
	if (g_engine->m_scriptSystem->HasProjectScripts() && !g_engine->m_scriptSystem->IsProjectAssemblyLoaded())
	{
		ERR_PRINT("Cannot run C# project: Game.dll could not be loaded. Build the net9.0 project first.");
		m_exitCode = 1;
		m_shouldQuit = true;
		return;
	}

	m_clock                = new Clock();
	m_sceneTree            = MemNew<SceneTree>();
	VirtualPath const path = ProjectSettings::Get()->m_startScenePath;
	Ref<PackedScene>  scene;
	if (path.IsValid())
	{
		scene = Ref<PackedScene>(ResourceLoader::LoadUncached(path));
	}
	Node* root = scene.IsValid() ? scene->Instantiate() : nullptr;
	if (root == nullptr)
	{
		ERR_PRINT("Cannot run project: no valid main scene. " + path.GetString());
		m_exitCode   = 1;
		m_shouldQuit = true;
		return;
	}
	m_sceneTree->ChangeScene(root);
	m_sceneTree->FlushPendingNode();
	INFO_PRINT("Running main scene: " + path.GetString());
}

void App::ShutdownScene()
{
	m_projectManagerErrorLabel = nullptr;
	m_projectManagerRoot       = nullptr;
	m_createProjectPopup       = nullptr;
	m_recentTree               = nullptr;
	m_editProjectButton        = nullptr;
	m_removeProjectButton      = nullptr;
	m_recentAvailable.clear();
	m_projectSearch = nullptr;
	m_recentDirty   = true;
	m_editorCamera  = nullptr;
	m_isSlowMode    = false;

	MemDelete(m_sceneTree);
	m_sceneTree = nullptr;

	delete m_clock;
	m_clock = nullptr;
}

int MingEngine::Run(MingRunConfig const& config)
{
	if (config.generateCSharpBindings)
	{
		ClassDatabase::Startup();
		RegisterAllTypes();

		CSharpScriptGenerator csharpGenerator;
		bool const            success = csharpGenerator.GenerateCSharpBindings(config.csharpBindingsOutputDirectory);
		GUARANTEE_OR_DIE(success, "Failed to generate C# bindings.");

		ThemeDatabase::Shutdown();
		ClassDatabase::Shutdown();
		return 0;
	}

	App app(config);
	g_app = &app;

	app.Startup();
	app.RunMainLoop();
	app.Shutdown();

	g_app = nullptr;
	return app.GetExitCode();
}

void App::StopGame()
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

void App::PollGame()
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

void App::ReadGameOutput()
{
	if (m_gameOutputRead == nullptr)
	{
		return;
	}

	DWORD available = 0;
	char buffer[4096];
	while (PeekNamedPipe(m_gameOutputRead, nullptr, 0, nullptr, &available, nullptr) && available > 0)
	{
		DWORD read = 0;
		if (!ReadFile(m_gameOutputRead, buffer, (std::min)(available, static_cast<DWORD>(sizeof(buffer))), &read, nullptr) || read == 0)
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

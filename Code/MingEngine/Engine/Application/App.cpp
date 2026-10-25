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
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Script/CSharpScript.hpp"
#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/imgui/imgui.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commdlg.h>
#include <string>

#pragma comment(lib, "Comdlg32.lib")

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
	ClassDatabase::Startup();

	RegisterAllTypes();

	g_engine->Startup();

	if (m_runConfig.mode == MingRunMode::Editor)
	{
		EditorIcons::Startup();
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
		glfwSetWindowSize(window, 720, 360);
	}

	m_quitEventID = RegisterEvent("Quit", App::OnQuit);
}

void App::Shutdown()
{
	ShutdownScene();

	if (g_engine != nullptr && g_engine->m_eventSystem != nullptr)
	{
		UnregisterEvent("Quit", m_quitEventID);
		m_quitEventID = 0;
	}

	ResourceLoader::Shutdown();

	EditorIcons::Shutdown();

	g_engine->Shutdown();

	ClassDatabase::Shutdown();
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
	if (m_runConfig.mode == MingRunMode::ProjectManager)
	{
		DrawProjectManager();
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
		if (m_uiResizeSizeLabel != nullptr)
		{
			IntVec2 const size = static_cast<Viewport*>(m_sceneTree->GetRoot())->GetOutputResolution();
			m_uiResizeSizeLabel->SetText(Stringf("Viewport: %d x %d", size.x, size.y).c_str());
		}
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

void App::Restart() { m_shouldRestart = true; }

void App::Quit() { m_shouldQuit = true; }

bool App::LaunchGame()
{
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

	PROCESS_INFORMATION processInfo{};

	// 4) Start the game process
	if (!CreateProcessW(
			executablePath.c_str(),
			commandLine.data(),
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,
			nullptr,
			&startupInfo,
			&processInfo))
	{
		DWORD const       errorCode = GetLastError();
		std::string const message   = "Failed to launch game. Windows error: " + std::to_string(errorCode) + "\n";

		ERR_PRINT(message.c_str());
		return false;
	}

	// 5) Release the launch-only process handles
	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
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

void App::DrawProjectManager()
{
	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.f, 24.f));
	if (ImGui::Begin(
			"Project Manager",
			nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::TextUnformatted("MingEngine");
		ImGui::Spacing();
		ImGui::TextWrapped("Select project.ming to open a project.");
		ImGui::Spacing();
		if (ImGui::Button("Open Project...", ImVec2(240.f, 42.f)))
		{
			OpenProject();
		}
		if (!m_projectManagerError.empty())
		{
			ImGui::Spacing();
			ImGui::TextWrapped("%s", m_projectManagerError.c_str());
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void App::OpenProject()
{
	m_projectManagerError.clear();

	// 1) Select an existing project settings file without changing the working directory
	std::wstring  selectedPath(32768, L'\0');
	OPENFILENAMEW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner   = static_cast<HWND>(g_engine->m_windowSystem->GetHwnd());
	dialog.lpstrFilter = L"Ming project (project.ming)\0*.ming\0\0";
	dialog.lpstrFile   = selectedPath.data();
	dialog.nMaxFile    = static_cast<DWORD>(selectedPath.size());
	dialog.lpstrTitle  = L"Open Ming Project";
	dialog.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
	if (!GetOpenFileNameW(&dialog))
	{
		DWORD const error = CommDlgExtendedError();
		if (error != 0)
		{
			m_projectManagerError = "Failed to open the file dialog. Windows error: " + std::to_string(error);
		}
		return;
	}

	// 2) Validate the selected file before launching the editor
	std::filesystem::path const settingsPath(selectedPath.c_str());
	std::error_code             error;
	if (settingsPath.filename() != L"project.ming" || !std::filesystem::is_regular_file(settingsPath, error) || error)
	{
		m_projectManagerError = "Please select an existing project.ming file.";
		return;
	}

	std::wstring executablePath(32768, L'\0');
	DWORD const  length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
	if (length == 0 || length >= executablePath.size())
	{
		m_projectManagerError = "Failed to get the executable path.";
		return;
	}
	executablePath.resize(length);

	// 3) Launch the editor with an absolute project path and close the manager only on success
	std::wstring const projectArgument = (settingsPath.parent_path() / L".").wstring();
	std::wstring       commandLine     = L"\"" + executablePath + L"\" --editor --project \"" + projectArgument + L"\"";
	std::wstring const workingDirectory = std::filesystem::path(executablePath).parent_path().wstring();
	STARTUPINFOW       startupInfo{};
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION processInfo{};
	if (!CreateProcessW(
			executablePath.c_str(),
			commandLine.data(),
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,
			workingDirectory.c_str(),
			&startupInfo,
			&processInfo))
	{
		m_projectManagerError = "Failed to launch the editor. Windows error: " + std::to_string(GetLastError());
		return;
	}

	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	Quit();
}

void App::StartupEditor()
{
	m_clock     = new Clock();
	m_sceneTree = MemNew<SceneTree>();

	auto editorNode = new EditorNode();
	editorNode->SetName("EditorNode");
	m_sceneTree->GetRoot()->AddNode(editorNode);
	VirtualPath const& startScenePath = ProjectSettings::Get()->m_startScenePath;
	if (startScenePath.IsValid())
	{
		editorNode->LoadScene(startScenePath);
	}
}

void App::StartupGame()
{
	m_clock     = new Clock();
	m_sceneTree = MemNew<SceneTree>();

	// 1) Load the font and build the visual test helpers
	Ref<FontResource> font = ResourceLoader::Load("res://fusion-pixel-12px-proportional-zh_hans.ttf");
	if (!font.IsValid())
	{
		WARN_PRINT("UI layout test needs res://fusion-pixel-12px-proportional-zh_hans.ttf.");
	}

	auto addRect = [](Node* parent, char const* name, Color const& color, Vector2 const& minimum)
	{
		ColorRect* rect = MemNew<ColorRect>();
		rect->SetName(name);
		rect->SetColor(color);
		rect->SetCustomMinimumSize(minimum);
		parent->AddNode(rect);
		return rect;
	};

	auto addLabel = [&font](Node* parent, char const* name, char const* text, int fontSize)
	{
		Label* label = MemNew<Label>();
		label->SetName(name);
		label->SetFont(font);
		label->SetFontSize(fontSize);
		label->SetText(text);
		parent->AddNode(label);
		return label;
	};

	// 2) Build three nested margins that follow the root Viewport in both axes
	MarginContainer* example = MemNew<MarginContainer>();
	example->SetName("UIResizeTest");
	example->SetMargins(24.0f, 24.0f, 24.0f, 24.0f);
	addRect(example, "OuterBackground", Color(40, 80, 125), Vector2::Zero);

	MarginContainer* nested = MemNew<MarginContainer>();
	nested->SetName("MiddleMargin");
	nested->SetMargins(16.0f, 16.0f, 16.0f, 16.0f);
	example->AddNode(nested);
	addRect(nested, "MiddleBackground", Color(35, 125, 110), Vector2::Zero);

	MarginContainer* content = MemNew<MarginContainer>();
	content->SetName("InnerMargin");
	content->SetMargins(16.0f, 16.0f, 16.0f, 16.0f);
	nested->AddNode(content);
	addRect(content, "InnerBackground", Color(25, 30, 40), Vector2::Zero);

	VBoxContainer* column = MemNew<VBoxContainer>();
	column->SetName("Content");
	column->SetSeparation(12.0f);
	content->AddNode(column);

	// 3) Compare fill and shrink alignment inside visible allocated rectangles
	Label* title = addLabel(column, "Title", "Ming UI Size Flags Test", 28);
	title->SetHorizontalAlignment(HorizontalAlignment::Center);
	Label* hint = addLabel(column, "Hint", "Resize window. Dark areas show allocated rectangles.", 20);
	hint->SetHorizontalAlignment(HorizontalAlignment::Center);
	m_uiResizeSizeLabel = addLabel(column, "ViewportSize", "Viewport", 20);
	m_uiResizeSizeLabel->SetHorizontalAlignment(HorizontalAlignment::Center);

	auto addAlignmentRow = [&](char const* name, bool vertical)
	{
		HBoxContainer* row = MemNew<HBoxContainer>();
		row->SetName(name);
		row->SetSeparation(12.0f);
		if (vertical)
		{
			row->SetVerticalSizeFlags(Control::Size_ExpandFill);
		}
		column->AddNode(row);

		char const* const names[] = {"Fill", "Shrink Begin", "Shrink Center", "Shrink End"};
		int const flags[] = {Control::Size_Fill, Control::Size_ShrinkBegin, Control::Size_ShrinkCenter, Control::Size_ShrinkEnd};
		Color const colors[] = {Color(70, 145, 235), Color(220, 80, 80), Color(80, 190, 120), Color(220, 180, 70)};
		for (int index = 0; index < 4; ++index)
		{
			VBoxContainer* group = MemNew<VBoxContainer>();
			group->SetName(names[index]);
			group->SetSeparation(4.0f);
			group->SetHorizontalSizeFlags(Control::Size_ExpandFill);
			row->AddNode(group);
			addLabel(group, "Caption", names[index], 16);

			MarginContainer* slot = MemNew<MarginContainer>();
			slot->SetName("AllocatedArea");
			slot->SetCustomMinimumSize(Vector2(96.0f, vertical ? 80.0f : 36.0f));
			slot->SetVerticalSizeFlags(Control::Size_ExpandFill);
			group->AddNode(slot);
			addRect(slot, "Background", Color(45, 55, 70), Vector2::Zero);

			ColorRect* sample = addRect(slot, "Sample", colors[index], Vector2(48.0f, 24.0f));
			sample->SetHorizontalSizeFlags(vertical ? Control::Size_ShrinkCenter : flags[index]);
			sample->SetVerticalSizeFlags(vertical ? flags[index] : Control::Size_ShrinkCenter);
		}
	};

	addLabel(column, "HorizontalHint", "Horizontal alignment", 18);
	addAlignmentRow("HorizontalAlignment", false);
	addLabel(column, "VerticalHint", "Vertical alignment (this row expands vertically)", 18);
	addAlignmentRow("VerticalAlignment", true);

	// 4) Compare fixed width, expand with fill, and expand with shrink center
	addLabel(column, "ExpandHint", "HBox: fixed red | Expand + Fill green | Expand + Center yellow", 18);
	MarginContainer* expandArea = MemNew<MarginContainer>();
	expandArea->SetName("ExpandArea");
	expandArea->SetMargins(8.0f, 8.0f, 8.0f, 8.0f);
	column->AddNode(expandArea);
	addRect(expandArea, "Background", Color(45, 55, 70), Vector2::Zero);

	HBoxContainer* expandRow = MemNew<HBoxContainer>();
	expandRow->SetName("ExpandComparison");
	expandRow->SetSeparation(12.0f);
	expandArea->AddNode(expandRow);
	addRect(expandRow, "Fixed", Color(220, 80, 80), Vector2(80.0f, 28.0f));
	ColorRect* expandFill = addRect(expandRow, "ExpandFill", Color(80, 190, 120), Vector2(80.0f, 28.0f));
	expandFill->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	ColorRect* expandCenter = addRect(expandRow, "ExpandCenter", Color(220, 180, 70), Vector2(80.0f, 28.0f));
	expandCenter->SetHorizontalSizeFlags(Control::Size_Expand | Control::Size_ShrinkCenter);

	Label* corner = addLabel(content, "BottomRight", "BOTTOM RIGHT | fixed margins", 20);
	corner->SetHorizontalAlignment(HorizontalAlignment::Right);
	corner->SetVerticalAlignment(VerticalAlignment::Bottom);

	m_sceneTree->GetRoot()->AddNode(example);
}

void App::ShutdownScene()
{
	m_uiResizeSizeLabel = nullptr;
	m_editorCamera = nullptr;
	m_isSlowMode   = false;

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

		ClassDatabase::Shutdown();
		return 0;
	}

	App app(config);
	g_app = &app;

	app.Startup();
	app.RunMainLoop();
	app.Shutdown();

	g_app = nullptr;
	return 0;
}

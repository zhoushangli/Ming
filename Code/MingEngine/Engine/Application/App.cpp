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
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

#include "ThirdParty/GLFW/glfw3.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <windows.h>

App* g_app = nullptr;

App::App(MingRunConfig const& config) : m_runConfig(config)
{
	EngineConfig engineConfig;
	engineConfig.m_windowConfig.m_clientAspect     = 16.f / 9.f;
	engineConfig.m_windowConfig.m_appName          = "MingEngine";
	engineConfig.m_fileSystemConfig.m_resourceRoot = config.projectPath;

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

	RegisterEvent("Quit", App::OnQuit);
}

void App::Shutdown()
{
	ShutdownScene();

	if (g_engine != nullptr && g_engine->m_eventSystem != nullptr)
	{
		UnregisterEvent("Quit", App::OnQuit);
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

void App::BeginFrame() { g_engine->BeginFrame(); }

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
	StartupEditor();
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

	// 1) Build the standalone margin and horizontal box examples
	Control* example = MemNew<Control>();
	example->SetName("ContainerExample");

	auto addBackdrop = [example](char const* name, Vector2 const& position, Vector2 const& size)
	{
		ColorRect* backdrop = MemNew<ColorRect>();
		backdrop->SetName(name);
		backdrop->SetColor(Color(40, 48, 64));
		backdrop->SetPosition(position);
		backdrop->SetSize(size);
		example->AddNode(backdrop);
	};

	auto addRect = [](Node* parent, char const* name, Color const& color, Vector2 const& minimum)
	{
		ColorRect* rect = MemNew<ColorRect>();
		rect->SetName(name);
		rect->SetColor(color);
		rect->SetCustomMinimumSize(minimum);
		parent->AddNode(rect);
	};

	addBackdrop("MarginBackdrop", Vector2(40.0f, 80.0f), Vector2(300.0f, 140.0f));
	MarginContainer* margin = MemNew<MarginContainer>();
	margin->SetName("Margin");
	margin->SetPosition(Vector2(40.0f, 80.0f));
	margin->SetMargins(20.0f, 12.0f, 24.0f, 16.0f);
	addRect(margin, "Child", Color::Green, Vector2(80.0f, 40.0f));
	margin->SetSize(Vector2(300.0f, 140.0f));
	example->AddNode(margin);

	addBackdrop("BoxBackdrop", Vector2(40.0f, 260.0f), Vector2(360.0f, 80.0f));
	HBoxContainer* box = MemNew<HBoxContainer>();
	box->SetName("Box");
	box->SetPosition(Vector2(40.0f, 260.0f));
	box->SetSeparation(20.0f);
	addRect(box, "First", Color::Red, Vector2(80.0f, 40.0f));
	addRect(box, "Second", Color::Green, Vector2(100.0f, 60.0f));
	box->SetSize(Vector2(360.0f, 80.0f));
	example->AddNode(box);

	// 2) Build Margin -> VBox -> Label + HBox
	Ref<FontResource> font = ResourceLoader::Load("res://fusion-pixel-12px-proportional-zh_hans.ttf");
	if (!font.IsValid())
	{
		WARN_PRINT("Container example needs res://fusion-pixel-12px-proportional-zh_hans.ttf.");
	}

	MarginContainer* nested = MemNew<MarginContainer>();
	nested->SetName("Nested");
	nested->SetPosition(Vector2(460.0f, 80.0f));
	nested->SetMargins(16.0f, 12.0f, 16.0f, 12.0f);

	VBoxContainer* column = MemNew<VBoxContainer>();
	column->SetName("VBox");
	column->SetSeparation(12.0f);
	nested->AddNode(column);

	Label* label = MemNew<Label>();
	label->SetName("Label");
	label->SetFont(font);
	label->SetFontSize(32);
	label->SetText("Ming UI");
	column->AddNode(label);

	HBoxContainer* row = MemNew<HBoxContainer>();
	row->SetName("HBox");
	row->SetSeparation(20.0f);
	addRect(row, "First", Color::Red, Vector2(80.0f, 40.0f));
	addRect(row, "Second", Color::Green, Vector2(100.0f, 60.0f));
	column->AddNode(row);

	nested->SetSize(nested->GetCombinedMinimumSize());
	addBackdrop("NestedBackdrop", nested->GetLocalPosition(), nested->GetSize());
	example->AddNode(nested);

	// 3) Add static example titles
	auto addLabel = [example, &font](char const* name, char const* text, Vector2 const& position)
	{
		Label* caption = MemNew<Label>();
		caption->SetName(name);
		caption->SetFont(font);
		caption->SetFontSize(32);
		caption->SetText(text);
		caption->SetPosition(position);
		example->AddNode(caption);
	};

	addLabel("MarginTitle", "MarginContainer", Vector2(40.0f, 48.0f));
	addLabel("BoxTitle", "HBoxContainer", Vector2(40.0f, 228.0f));
	addLabel("NestedTitle", "Margin -> VBox -> Label + HBox", Vector2(460.0f, 48.0f));

	m_sceneTree->GetRoot()->AddNode(example);
}

void App::ShutdownScene()
{
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

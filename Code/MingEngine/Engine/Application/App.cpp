#include "MingEngine/Engine/Application/App.hpp"

#include "MingEngine/Core/Clock.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
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
#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/EngineService/RenderService.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"

#include "ThirdParty/GLFW/glfw3.h"

App* g_app = nullptr;

App::App(MingRunConfig const& config) : m_runConfig(config)
{
	EngineConfig engineConfig;
	engineConfig.m_windowConfig.m_clientAspect     = 16.f / 9.f;
	engineConfig.m_windowConfig.m_appName          = "MingEngine";
	engineConfig.m_fileSystemConfig.m_resourceRoot = config.projectPath;

	DevConsoleConfig consoleConfig;
	consoleConfig.m_isEnable = false;

	g_engine        = new Engine(engineConfig);
	g_engineService = new EngineService(consoleConfig);
}

App::~App()
{
	ShutdownScene();

	delete g_engineService;
	g_engineService = nullptr;

	delete g_engine;
	g_engine = nullptr;
}

void App::Startup()
{
	ClassDatabase::Startup();

	RegisterAllTypes();

	g_engine->Startup();
	g_engineService->Startup();

	EditorIcons::Startup();

	StartupScene();
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

	g_engineService->Shutdown();
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
	if (g_engineService != nullptr)
	{
		g_engineService->Render();
	}
}

void App::BeginFrame()
{
	g_engine->BeginFrame();
	if (g_engineService != nullptr)
	{
		g_engineService->BeginFrame();
	}
}

void App::EndFrame()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->FlushPendingNode();
	}

	if (g_engineService != nullptr)
	{
		g_engineService->EndFrame();
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
	StartupScene();
}

void App::StartupScene()
{
	m_clock     = new Clock();
	m_sceneTree = new SceneTree();

	auto editorNode = new EditorNode();
	editorNode->SetName("EditorNode");
	m_sceneTree->GetRoot()->AddNode(editorNode);
	VirtualPath const& startScenePath = ProjectSettings::Get()->m_startScenePath;
	if (startScenePath.IsValid())
	{
		editorNode->LoadScene(startScenePath);
	}
}

void App::ShutdownScene()
{
	m_editorCamera = nullptr;
	m_isSlowMode   = false;

	delete m_sceneTree;
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

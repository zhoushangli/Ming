#include "MingEngine/Engine/Application/App.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Console/DevConsole.hpp"
#include "MingEngine/Engine/Core/Clock.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"
#include "MingEngine/Engine/Render/Rgba8.hpp"
#include "MingEngine/Engine/Window/Window.hpp"
#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/EngineService/RenderService.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"

#if defined(MING_EDITOR)

#include "MingEngine/Editor/EditorController.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"

#endif

#include "ThirdParty/GLFW/glfw3.h"

App* g_app = nullptr;

App::App(IProjectModule& project, MingRunConfig const& config) : m_project(project), m_runConfig(config)
{
	EngineConfig engineConfig;
	engineConfig.m_windowConfig.m_clientAspect = m_runConfig.m_windowAspect;
	engineConfig.m_windowConfig.m_appName      = m_project.GetProjectName();
	engineConfig.m_devConsoleConfig.m_fontName = "pixel_operator";

	g_engine        = new Engine(engineConfig);
	g_engineService = new EngineService();

	g_engine->Startup();
	g_engineService->Startup();
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

#if defined(MING_EDITOR)
	ClassDatabase::RegisterClass<EditorNode>(false);
	ClassDatabase::RegisterClass<EditorGizmos>(false);
	ClassDatabase::RegisterClass<EditorController>(false);
#endif

	m_project.RegisterTypes();
	m_project.Startup();

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_engine->m_renderer;
	debugRenderConfig.m_fontName = "pixel_operator";
	DebugRenderSystemStartup(debugRenderConfig);

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

	DebugRenderSystemShutdown();
	m_project.Shutdown();
	ClassDatabase::Shutdown();

	g_engineService->Shutdown();
	g_engine->Shutdown();
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
	GLFWwindow* window       = g_engine->m_window->GetGLFWWindow();
	bool const hasFocus      = window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED);
	bool const isConsoleOpen = g_engine->m_devConsole != nullptr && g_engine->m_devConsole->IsOpen();

	if (!hasFocus || isConsoleOpen)
	{
		g_engine->m_input->SetCursorMode(CursorMode::POINTER);
		if (!hasFocus)
		{
			g_engine->m_input->ClearCursorDelta();
		}
	}

	if (g_engine->m_input->WasKeyJustPressed(KeyCodeF8))
	{
		Restart();
	}

	if (g_engine->m_input->WasKeyJustPressed(KeyCodeEsc))
	{
		FireEvent("Quit");
	}

	float const sceneDeltaSeconds = m_clock != nullptr ? static_cast<float>(m_clock->GetDeltaSeconds()) : deltaSeconds;
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->UpdateScene(sceneDeltaSeconds);
	}
}

void App::Render() const
{
	if (g_engineService != nullptr && g_engineService->m_renderService != nullptr)
	{
		g_engineService->m_renderService->Render();
	}
}

void App::BeginFrame()
{
	g_engine->BeginFrame();
	DebugRenderBeginFrame();
}

void App::EndFrame()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->FlushPendingNode();
	}

	g_engine->EndFrame();
	DebugRenderEndFrame();

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

#if defined(MING_EDITOR)

	m_editorController = new EditorController();
	m_editorController->SetName("EditorController");
	m_sceneTree->GetRoot()->AddNode(m_editorController);

	auto editorNode = new EditorNode();
	editorNode->SetName("EditorNode");
	m_sceneTree->GetRoot()->AddNode(editorNode);

#endif

	PackedScene packedScene;
	packedScene.LoadFromFile("EditorSavedScene.json");

	Node* newSceneRoot = packedScene.Instantiate();

	if (newSceneRoot != nullptr)
	{
		m_sceneTree->ChangeScene(newSceneRoot);
	}
}

void App::ShutdownScene()
{
#if defined(MING_EDITOR)
	m_editorController = nullptr;
	m_isSlowMode       = false;
#endif

	delete m_sceneTree;
	m_sceneTree = nullptr;

	delete m_clock;
	m_clock = nullptr;
}

int MingEngine::Run(IProjectModule& project, MingRunConfig const& config)
{
	App app(project, config);
	g_app = &app;

	app.Startup();
	app.RunMainLoop();
	app.Shutdown();

	g_app = nullptr;
	return 0;
}

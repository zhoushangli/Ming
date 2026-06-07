#include "MingEngine/Engine/Application/App.hpp"

#include "MingEngine/Engine/Core/Clock.hpp"
#include "MingEngine/Engine/Console/DevConsole.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Editor/EditorApplication.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Window/Window.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"
#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/EngineService/RenderService.hpp"

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
	delete m_stateMachine;
	m_stateMachine = nullptr;

	delete g_engineService;
	g_engineService = nullptr;

	delete g_engine;
	g_engine = nullptr;
}

void App::Startup()
{
	ClassDatabase::Startup();
	RegisterEditorTypes();

	m_project.RegisterTypes();
	m_project.Startup();

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_engine->m_renderer;
	debugRenderConfig.m_fontName = "pixel_operator";
	DebugRenderSystemStartup(debugRenderConfig);

	m_stateMachine = new AppStateMachine();
	m_stateMachine->Startup(AppStateType::Editor, m_project);
	
	RegisterEvent("Quit", App::OnQuit);
}

void App::Shutdown()
{
	if (m_stateMachine != nullptr)
	{
		m_stateMachine->Shutdown();
	}

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
	GLFWwindow* window        = g_engine->m_window->GetGLFWWindow();
	bool const  hasFocus      = window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED);
	bool const  isConsoleOpen = g_engine->m_devConsole != nullptr && g_engine->m_devConsole->IsOpen();

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

	if (m_stateMachine != nullptr)
	{
		m_stateMachine->Update(deltaSeconds);
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

	if (m_stateMachine != nullptr)
	{
		m_stateMachine->BeginFrame();
	}
}

void App::EndFrame()
{
	if (m_stateMachine != nullptr)
	{
		m_stateMachine->EndFrame();
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

void App::Restart()
{
	m_shouldRestart = true;
}

void App::Quit()
{
	m_shouldQuit = true;
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
	delete m_stateMachine;
	m_stateMachine = new AppStateMachine();
	m_stateMachine->Startup(AppStateType::Editor, m_project);
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
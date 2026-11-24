#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Engine/Application/App.hpp"

#include "MingEngine/Core/Clock.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Engine/Application/AppState.hpp"
#include "MingEngine/Engine/Application/EditorState.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/GameState.hpp"
#include "MingEngine/Engine/Application/ProjectManagerState.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>

#pragma comment(lib, "Ole32.lib")

App* g_app = nullptr;

App::App(MingRunConfig const& config) : m_runConfig(config)
{
	EngineConfig engineConfig;
	engineConfig.m_enablePhysics = config.mode == MingRunMode::Game;
	engineConfig.m_enableWorkerThreadPool = config.mode == MingRunMode::Game || config.mode == MingRunMode::Editor;
	engineConfig.m_windowConfig.m_clientAspect = 16.f / 9.f;
	engineConfig.m_windowConfig.m_appName = config.mode == MingRunMode::ProjectManager
		? "MingEngine - Project Manager" : "MingEngine";
	engineConfig.m_fileSystemConfig.m_resourceRoot = config.projectPath;
	m_engine = std::make_unique<Engine>(engineConfig);
	m_engine->SetEditorMode(config.mode == MingRunMode::Editor);
	g_app = this;
}

App::~App()
{
	Shutdown();
	m_engine.reset();
	g_engine = nullptr;
	g_app = nullptr;
}

void App::Startup()
{
	if (m_started)
	{
		return;
	}

	m_started = true;

	// 1) Initialize the UI COM apartment before the audio backend.
	HRESULT const initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	m_comInitialized = SUCCEEDED(initialized);
	if (!m_comInitialized)
	{
		WARN_PRINT(Stringf("Could not initialize the UI COM apartment (HRESULT 0x%08X).", initialized));
	}

	ClassDatabase::Startup();
	RegisterAllTypes();
	if (!m_engine->Startup())
	{
		m_exitCode = 1;
		m_shouldQuit = true;
		return;
	}

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

			m_exitCode = 1;
			m_shouldQuit = true;
			return;
		}
	}

	EnterState();
}

void App::Shutdown()
{
	if (!m_started)
	{
		return;
	}

	if (EditorNode::Get() != nullptr)
	{
		EditorNode::Get()->ShutdownBackgroundWork();
	}
	m_engine->StopWorkerThreadPool();
	ExitState();

	ProjectSettings::Reset();
	ThemeDatabase::Shutdown();
	ResourceLoader::Shutdown();
	EditorIcons::Shutdown();
	m_engine->Shutdown();
	ClassDatabase::Shutdown();

	if (m_comInitialized)
	{
		CoUninitialize();
		m_comInitialized = false;
	}

	m_started = false;
}

void App::EnterState()
{
	// Select the mode for this process at startup. Opening another mode launches a new instance.
	// e.g. Editor Play launches a separate Game process with its own engine and physics world.
	switch (m_runConfig.mode)
	{
	case MingRunMode::ProjectManager:
		m_state = std::make_unique<ProjectManagerState>();
		break;
	case MingRunMode::Editor:
		m_state = std::make_unique<EditorState>();
		break;
	case MingRunMode::Game:
		m_state = std::make_unique<GameState>();
		break;
	}

	m_clock = std::make_unique<Clock>();
	if (m_state == nullptr || !m_state->Enter(*this))
	{
		m_exitCode = 1;
		m_shouldQuit = true;
		ExitState();
	}
}

void App::ExitState()
{
	if (m_state != nullptr)
	{
		m_state->Exit(*this);
	}

	// 1) Destroy nodes and their captured callbacks while the state still exists.
	m_engine->m_sceneSystem->Shutdown();
	m_state.reset();
	m_clock.reset();
}

void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::RunFrame()
{
	if (!m_started || m_shouldQuit || m_state == nullptr)
	{
		return;
	}

	m_engine->BeginFrame();
	Clock::TickSystemClock();
	float const deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
	if (!m_engine->Update(deltaSeconds, [this]() { return IsQuitting(); }))
	{
		Quit();
	}

	m_engine->Render();
	m_engine->EndFrame();
}

void App::Quit()
{
	if (m_state != nullptr && !m_state->CanQuit(*this))
	{
		return;
	}

	m_shouldQuit = true;
}

bool App::LaunchGame()
{
	auto* editor = dynamic_cast<EditorState*>(m_state.get());
	return editor != nullptr && editor->LaunchGame();
}

bool App::IsGameRunning() const
{
	auto* editor = dynamic_cast<EditorState*>(m_state.get());
	return editor != nullptr && editor->IsGameRunning();
}

void App::StopGame()
{
	if (auto* editor = dynamic_cast<EditorState*>(m_state.get()))
	{
		editor->StopGame();
	}
}

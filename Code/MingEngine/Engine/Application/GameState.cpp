#include "MingEngine/Engine/Application/GameState.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Scene/SceneSystem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <cmath>

bool GameState::Enter([[maybe_unused]] App& app)
{
	m_physicsAccumulator = 0.0;
	m_running = false;
	if (!g_engine->m_physicsServer->Startup())
	{
		ERR_PRINT("Cannot start the physics world.");
		return false;
	}

	if (g_engine->m_scriptSystem->HasProjectScripts() && !g_engine->m_scriptSystem->IsProjectAssemblyLoaded())
	{
		ERR_PRINT("Cannot run C# project: Game.dll could not be loaded. Build the net9.0 project first.");
		return false;
	}

	std::string error;
	SceneSystem* scenes = g_engine->m_sceneSystem;
	if (!scenes->CreateAutoloads(ProjectSettings::Get()->m_autoLoads, error))
	{
		ERR_PRINT(error);
		return false;
	}

	VirtualPath const path = ProjectSettings::Get()->m_startScenePath;
	if (!scenes->ChangeScene(path, error))
	{
		ERR_PRINT(error);
		return false;
	}

	scenes->EndFrame();
	m_running = true;
	INFO_PRINT("Running main scene: " + path.GetString());
	return true;
}

void GameState::Update(App& app, float deltaSeconds)
{
	if (!g_engine->m_inputSystem->IsWindowFocused())
	{
		g_engine->m_inputSystem->ClearCursorDelta();
	}

	if (!m_running || app.IsQuitting() || !std::isfinite(deltaSeconds) || deltaSeconds < 0.f)
	{
		return;
	}

	constexpr double fixedDelta = 1.0 / 60.0;
	constexpr int maxSteps = 8;
	m_physicsAccumulator += static_cast<double>(deltaSeconds);
	int steps = 0;
	while (m_physicsAccumulator >= fixedDelta && steps < maxSteps)
	{
		g_engine->m_sceneSystem->PhysicsProcess(static_cast<float>(fixedDelta));
		if (!g_engine->m_physicsServer->Step(static_cast<float>(fixedDelta)))
		{
			ERR_PRINT("Physics simulation failed; stopping the game.");
			m_running = false;
			app.Quit();
			break;
		}

		m_physicsAccumulator -= fixedDelta;
		++steps;
		if (app.IsQuitting())
		{
			break;
		}
	}

	if (steps == maxSteps && m_physicsAccumulator >= fixedDelta)
	{
		m_physicsAccumulator = std::fmod(m_physicsAccumulator, fixedDelta);
	}
}

void GameState::Exit([[maybe_unused]] App& app)
{
	m_running = false;
	m_physicsAccumulator = 0.0;
	// 1) App tears down the complete scene environment after the state exits.
}

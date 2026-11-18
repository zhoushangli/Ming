#include "MingEngine/Engine/Application/GameState.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Scene/SceneSystem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

bool GameState::Enter([[maybe_unused]] App& app)
{
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
	INFO_PRINT("Running main scene: " + path.GetString());
	return true;
}

void GameState::Exit([[maybe_unused]] App& app)
{
	// 1) App tears down the complete scene environment after the state exits.
}

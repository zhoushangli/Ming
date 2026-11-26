#include "MingEngine/Editor/EditorRunManager.hpp"
#include "MingEngine/Editor/EditorChecker.hpp"
#include "MingEngine/Editor/EditorSceneManager.hpp"
#include "MingEngine/Editor/EditorCSharpBuildManager.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

EditorRunManager::EditorRunManager(EditorNode& editor) : m_editor(editor)
{
}

void EditorRunManager::RequestPlay()
{
	if (g_app == nullptr || g_app->IsGameRunning() || IsPreparingPlay() || m_editor.IsBuildingCSharpProject() || m_editor.IsCheckingProject())
	{
		return;
	}

	if (!m_editor.GetChecker()->SavePendingChanges())
	{
		return;
	}

	m_editor.m_editorGizmos->CancelDrag();
	m_editor.m_editorUI->FinishNativeEditing();
	m_editor.m_sceneManager->CancelPendingSceneAction();
	m_state = State::Saving;
	auto& scenes = *m_editor.m_sceneManager;
	scenes.m_pendingSceneAction = EditorSceneManager::PendingSceneAction::Continue;
	scenes.m_afterClose = [this]() { m_state = State::Preparing; m_playDelayFrames = 1; };
	if (m_editor.HasScene() && !m_editor.SaveScene())
	{
		scenes.m_waitingForSave = scenes.m_lastSaveResult == EditorSceneManager::SaveResult::AwaitingPath;
		if (!scenes.m_waitingForSave)
		{
			m_editor.m_sceneManager->CancelPendingSceneAction();
		}
		return;
	}

	scenes.ExecutePendingSceneAction();
}

bool EditorRunManager::IsPreparingPlay() const
{
	return m_state != State::Idle;
}

void EditorRunManager::Cancel()
{
	m_state = State::Idle;
}

void EditorRunManager::BuildFinished(bool success)
{
	if (m_state == State::Building)
	{
		m_state = success ? State::Launching : State::Idle;
		m_playDelayFrames = 1;
	}
}

void EditorRunManager::Update()
{
	if (m_playDelayFrames > 0)
	{
		--m_playDelayFrames;
		return;
	}

	if (m_state == State::Preparing)
	{
		VirtualPath const main = ProjectSettings::Get()->m_startScenePath;
		Ref<PackedScene> scene;
		if (main.IsValid())
		{
			scene = Ref<PackedScene>(ResourceLoader::LoadUncached(main));
		}
		if (!scene.IsValid())
		{
			m_state = State::Saving;
			m_editor.m_editorUI->OpenSceneFile(SceneFilePopup::Mode::MainScene, m_editor.GetCurrentScenePath(),
				[this](VirtualPath const& path)
				{
					if (!m_editor.SetMainScene(path))
					{
						return false;
					}
					m_state = State::Preparing;
					return true;
				}, [this]() { Cancel(); });
			return;
		}

		bool const scripted = g_engine->m_scriptSystem->HasProjectScripts() || g_engine->m_fileSystem->Exists("res://Game.csproj");
		if (scripted)
		{
			m_state = State::Building;
			if (!m_editor.m_buildManager->BeginCSharpBuild())
			{
				Cancel();
			}
		}
		else
		{
			Cancel();
			LaunchPreparedGame(false);
		}
	}
	else if (m_state == State::Launching)
	{
		Cancel();
		LaunchPreparedGame(true);
	}
}

void EditorRunManager::LaunchPreparedGame(bool scripted)
{
	std::string error;
	if (scripted)
	{
		if (!g_engine->m_scriptSystem->ReloadProjectAssembly())
		{
			error = "Could not load the compiled C# assembly. See Output.";
		}
		else
		{
			Ref<PackedScene> main(ResourceLoader::LoadUncached(ProjectSettings::Get()->m_startScenePath));
			Node* root = main.IsValid() ? main->Instantiate() : nullptr;
			bool const valid = g_engine->m_scriptSystem->ValidateSceneScripts(root);
			MemDelete(root);
			if (!valid)
			{
				error = "A scene script has no compiled type or an incompatible base. See Output.";
			}
		}
	}

	m_editor.m_editorUI->HideBuildProgress();
	if (error.empty() && !g_app->LaunchGame())
	{
		error = "Could not start the game. See Output for details.";
	}
	if (!error.empty())
	{
		m_editor.m_editorUI->SetStatusMessage("Run failed.");
		m_editor.m_editorUI->Warning("Run Failed", error);
	}
}

void EditorRunManager::Stop()
{
	Cancel();
	if (g_app != nullptr)
	{
		g_app->StopGame();
	}
}

#pragma once

#include "MingEngine/Engine/Event/EventSystem.hpp"

#include <filesystem>

class Clock;
class EditorCamera;
class Node;
class SceneTree;

enum class MingRunMode
{
	Editor,
	Game
};

struct MingRunConfig
{
	MingRunMode mode = MingRunMode::Editor;
	std::filesystem::path projectPath;
};

namespace MingEngine
{
	int Run(MingRunConfig const &config);
} // namespace MingEngine

class App
{
public:
	explicit App(MingRunConfig const &config);
	~App();

	void Startup();
	void Shutdown();
	void RunMainLoop();
	void RunFrame();
	void Restart();
	void Quit();

	bool IsQuitting() const { return m_shouldQuit; }

	static bool OnQuit(EventArgs &args);

private:
	void Update(float deltaSeconds);
	void Render() const;
	void BeginFrame();
	void EndFrame();
	void RestartImmediately();
	void StartupScene();
	void ShutdownScene();

private:
	bool m_shouldRestart = false;
	bool m_shouldQuit = false;

	Clock *m_clock = nullptr;
	SceneTree *m_sceneTree = nullptr;
	MingRunConfig m_runConfig;

	EditorCamera *m_editorCamera = nullptr;
	bool m_isSlowMode = false;
};

extern App *g_app;

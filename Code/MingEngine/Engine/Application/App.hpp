#pragma once

#include "MingEngine/Engine/Event/EventSystem.hpp"

class Clock;
class EditorController;
class Node;
class SceneTree;

struct MingRunConfig
{
	float m_windowAspect = 16.f / 9.f;
};

class IProjectModule
{
public:
	virtual ~IProjectModule() = default;

	virtual char const* GetProjectName() const = 0;
	virtual void RegisterTypes()               = 0;
	virtual void Startup() {}
	virtual void Shutdown() {}
};

namespace MingEngine
{
int Run(IProjectModule& project, MingRunConfig const& config);
} // namespace MingEngine

class App
{
public:
	App(IProjectModule& project, MingRunConfig const& config);
	~App();

	void Startup();
	void Shutdown();
	void RunMainLoop();
	void RunFrame();
	void Restart();
	void Quit();

	bool IsQuitting() const { return m_shouldQuit; }

	static bool OnQuit(EventArgs& args);

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
	bool m_shouldQuit    = false;

	Clock*          m_clock     = nullptr;
	SceneTree*      m_sceneTree = nullptr;
	IProjectModule& m_project;
	MingRunConfig   m_runConfig;

#if defined(MING_EDITOR)
	EditorController* m_editorController = nullptr;
	bool              m_isSlowMode       = false;
#endif
};

extern App* g_app;

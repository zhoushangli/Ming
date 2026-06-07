#pragma once

#include "MingEngine/Engine/Event/EventSystem.hpp"

class AppStateMachine;
class Node;

enum class MingRunMode
{
	Editor,
};

struct MingRunConfig
{
	MingRunMode m_mode         = MingRunMode::Editor;
	float       m_windowAspect = 16.f / 9.f;
};

class IProjectModule
{
public:
	virtual ~IProjectModule() = default;

	virtual char const* GetProjectName() const = 0;
	virtual void        RegisterTypes()        = 0;
	virtual void        Startup() {}
	virtual void        Shutdown() {}
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

private:
	bool m_shouldRestart = false;
	bool m_shouldQuit    = false;

	AppStateMachine* m_stateMachine = nullptr;
	IProjectModule&  m_project;
	MingRunConfig    m_runConfig;
};

extern App* g_app;
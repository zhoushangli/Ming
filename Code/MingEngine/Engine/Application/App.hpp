#pragma once

#include "MingEngine/Engine/Application/MingRunConfig.hpp"

#include <memory>

class AppState;
class Clock;
class Engine;

class App
{
public:
	explicit App(MingRunConfig const& config);
	~App();

	App(App const&) = delete;
	App& operator=(App const&) = delete;

	void Startup();
	void Shutdown();
	void RunMainLoop();
	void RunFrame();
	void Quit();

	bool LaunchGame();
	bool IsGameRunning() const;
	void StopGame();

	int GetExitCode() const { return m_exitCode; }
	bool IsQuitting() const { return m_shouldQuit; }
	MingRunConfig const& GetRunConfig() const { return m_runConfig; }
	Engine* GetEngine() const { return m_engine.get(); }

private:
	void EnterState();
	void ExitState();

private:
	MingRunConfig m_runConfig;
	std::unique_ptr<Engine> m_engine;
	std::unique_ptr<AppState> m_state;
	std::unique_ptr<Clock> m_clock;
	bool m_started = false;
	bool m_shouldQuit = false;
	bool m_comInitialized = false;
	int m_exitCode = 0;
};

extern App* g_app;

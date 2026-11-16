#pragma once

#include "MingEngine/Engine/Application/AppState.hpp"
#include <string>

class EditorState : public AppState
{
public:
	bool Enter(App& app) override;
	void Update(App& app, float deltaSeconds) override;
	void Exit(App& app) override;
	bool CanQuit(App& app) override;

	bool LaunchGame();
	bool IsGameRunning() const { return m_gameProcess != nullptr; }
	void StopGame();

private:
	void UpdateGameProcess();
	void ReadGameOutput();

private:
	App*               m_app            = nullptr;
	void*              m_gameProcess    = nullptr;
	void*              m_gameOutputRead = nullptr;
	std::string        m_gameOutput;
	unsigned long      m_gameProcessID    = 0;
	unsigned long long m_gameStopDeadline = 0;
};

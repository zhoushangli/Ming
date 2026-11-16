#pragma once

#include "MingEngine/Engine/Application/AppState.hpp"

class GameState : public AppState
{
public:
	bool Enter(App& app) override;
	void Update(App& app, float deltaSeconds) override;
	void Exit(App& app) override;

private:
	double m_physicsAccumulator = 0.0;
	bool m_running = false;
};

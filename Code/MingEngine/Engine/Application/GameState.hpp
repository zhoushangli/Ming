#pragma once

#include "MingEngine/Engine/Application/AppState.hpp"

class GameState : public AppState
{
public:
	bool Enter(App& app) override;
	void Exit(App& app) override;
};

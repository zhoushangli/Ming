#pragma once

#include <Engine/Renderer/Renderer.hpp>

extern Engine* g_engine;

class Engine 
{
public:
	Engine();
	~Engine();

	void BegineFrame();
	void EndFrame();

public:
	Renderer* renderer = nullptr;
};
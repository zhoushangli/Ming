#pragma once

#include <vector>

class RenderService;

class EngineService
{
public:
	// EngineService is owned by App and outlives every SceneTree.
	EngineService();
	~EngineService();

	void Startup();
	void Shutdown();

	RenderService* m_renderService = nullptr;
};

extern EngineService* g_engineService;

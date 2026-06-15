#pragma once

#include "MingEngine/EngineService/DevConsole.hpp"

struct EngineConfig;
class RenderService;

class EngineService
{
public:
	// EngineService is owned by App and outlives every SceneTree.
	EngineService(DevConsoleConfig const& consoleConfig);
	~EngineService();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void Render() const;
	void EndFrame();

	RenderService* m_renderService = nullptr;
	DevConsole*    m_console       = nullptr;
};

extern EngineService* g_engineService;

#include "MingEngine/EngineService/EngineService.hpp"

#include "MingEngine/EngineService/RenderService.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

#include "EngineService.hpp"
#include <algorithm>

EngineService* g_engineService = nullptr;

EngineService::EngineService() { m_renderService = new RenderService(); }

EngineService::~EngineService()
{
	m_renderService->Shutdown();

	delete m_renderService;
	m_renderService = nullptr;

	if (g_engineService == this)
	{
		g_engineService = nullptr;
	}
}

void EngineService::Startup() { m_renderService->Startup(); }

void EngineService::Shutdown() { m_renderService->Shutdown(); }

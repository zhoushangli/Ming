#include "MingEngine/EngineService/EngineService.hpp"

#include "MingEngine/EngineService/RenderService.hpp"

EngineService* g_engineService = nullptr;

EngineService::EngineService(DevConsoleConfig const& consoleConfig)
{
	g_engineService = this;
	m_renderService = new RenderService();
	if (consoleConfig.m_isEnable)
	{
		m_console = new DevConsole(consoleConfig);
	}
}

EngineService::~EngineService()
{
	delete m_console;
	m_console = nullptr;

	delete m_renderService;
	m_renderService = nullptr;

	if (g_engineService == this)
	{
		g_engineService = nullptr;
	}
}

void EngineService::Startup()
{
	if (m_renderService != nullptr)
	{
		m_renderService->Startup();
	}

	if (m_console != nullptr)
	{
		m_console->Startup();
	}
}

void EngineService::Shutdown()
{
	if (m_console != nullptr)
	{
		m_console->Shutdown();
	}

	if (m_renderService != nullptr)
	{
		m_renderService->Shutdown();
	}
}

void EngineService::BeginFrame()
{
	if (m_console != nullptr)
	{
		m_console->BeginFrame();
	}
}

void EngineService::Render() const
{
	if (m_renderService != nullptr)
	{
		m_renderService->Render();
	}

	if (m_console != nullptr)
	{
		m_console->Render();
	}
}

void EngineService::EndFrame()
{
	if (m_console != nullptr)
	{
		m_console->EndFrame();
	}
}

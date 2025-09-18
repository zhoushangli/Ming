#include "Engine.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

Engine* g_engine = nullptr;

Engine::Engine()
{
	m_renderer = new Renderer();
	m_rng = new RandomNumberGenerator();

	m_renderer->Startup();
}

Engine::~Engine()
{
	m_renderer->Shutdown();

	delete m_renderer;
	m_renderer = nullptr;

	delete m_rng;
	m_rng = nullptr;
}

void Engine::BeginFrame()
{
	m_renderer->BeginFrame();
}

void Engine::EndFrame()
{
	m_renderer->EndFrame();
}

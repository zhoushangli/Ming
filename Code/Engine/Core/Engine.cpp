#include "Engine.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Input/InputSystem.hpp"

Engine* g_engine = nullptr;

Engine::Engine()
{
	m_renderer = new Renderer();
	m_inputSystem = new InputSystem();
	m_rng = new RandomNumberGenerator();

	m_renderer->Startup();
	m_inputSystem->Startup();
}

Engine::~Engine()
{
	m_renderer->Shutdown();
	m_inputSystem->Shutdown();

	delete m_renderer;
	m_renderer = nullptr;

	delete m_inputSystem;
	m_inputSystem = nullptr;

	delete m_rng;
	m_rng = nullptr;
}

void Engine::BeginFrame()
{
	m_renderer->BeginFrame();
	m_inputSystem->BeginFrame();
}

void Engine::EndFrame()
{
	m_renderer->EndFrame();
	m_inputSystem->EndFrame();
}

#include "Engine/Core/Engine.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"

Engine* g_engine = nullptr;

Engine::Engine()
{
	m_renderer = new Renderer();
	m_inputSystem = new InputSystem();
	m_audioSystem = new AudioSystem();

	m_renderer->Startup();
	m_inputSystem->Startup();
	m_audioSystem->Startup();
}

Engine::~Engine()
{
	m_audioSystem->Shutdown();
	m_inputSystem->Shutdown();
	m_renderer->Shutdown();

	delete m_audioSystem;
	m_audioSystem = nullptr;

	delete m_inputSystem;
	m_inputSystem = nullptr;

	delete m_renderer;
	m_renderer = nullptr;
}

void Engine::BeginFrame()
{
	m_renderer->BeginFrame();
	m_inputSystem->BeginFrame();
	m_audioSystem->BeginFrame();
}

void Engine::EndFrame()
{
	m_renderer->EndFrame();
	m_inputSystem->EndFrame();
	m_audioSystem->EndFrame();
}

#include "Engine/Core/Engine.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Window/Window.hpp"

Engine* g_engine = nullptr;

Engine::Engine()
{
	m_window	= new Window();
	m_renderer	= new Renderer();
	m_input		= new InputSystem();
	m_audio		= new AudioSystem();

	m_window->Startup();
	m_renderer->Startup();
	m_input->Startup();
	m_audio->Startup();
}

Engine::~Engine()
{
	m_audio->Shutdown();
	m_input->Shutdown();
	m_renderer->Shutdown();
	m_window->Shutdown();

	delete m_audio;
	m_audio = nullptr;

	delete m_input;
	m_input = nullptr;

	delete m_renderer;
	m_renderer = nullptr;

	delete m_window;
	m_window = nullptr;
}

void Engine::BeginFrame()
{
	m_window->BeginFrame();
	m_renderer->BeginFrame();
	m_input->BeginFrame();
	m_audio->BeginFrame();
}

void Engine::EndFrame()
{
	m_window->EndFrame();
	m_renderer->EndFrame();
	m_input->EndFrame();
	m_audio->EndFrame();
}

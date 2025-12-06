#pragma once

#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"

class RandomNumberGenerator;
class Window;
class Renderer;
class InputSystem;
class AudioSystem;

struct EngineConfig
{
    EventSystemConfig	m_eventSystemConfig;
    DevConsoleConfig	m_devConsoleConfig;
	WindowConfig		m_windowConfig;
	RendererConfig		m_rendererConfig;
	AudioConfig			m_audioConfig;
	InputConfig			m_inputConfig;
};

class Engine 
{
public:
	Engine(EngineConfig config);
	~Engine();

    void Startup();
    void Shutdown();
	void BeginFrame();
	void EndFrame();

public:
	EngineConfig	m_config;

    EventSystem*	m_eventSystem = nullptr;
    DevConsole*		m_devConsole  = nullptr;
	Window*			m_window	  = nullptr;
	Renderer*		m_renderer	  = nullptr;
	InputSystem*	m_input		  = nullptr;
	AudioSystem*	m_audio		  = nullptr;
};

extern Engine* g_engine;

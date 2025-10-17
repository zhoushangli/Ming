#pragma once

class RandomNumberGenerator;
class Window;
class Renderer;
class InputSystem;
class AudioSystem;

class Engine 
{
public:
	Engine();
	~Engine();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

public:
	Window*			m_window	= nullptr;
	Renderer*		m_renderer	= nullptr;
	InputSystem*	m_input		= nullptr;
	AudioSystem*	m_audio		= nullptr;
};

extern Engine* g_engine;

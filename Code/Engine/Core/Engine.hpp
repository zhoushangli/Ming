#pragma once

class RandomNumberGenerator;
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
	Renderer*				m_renderer		= nullptr;
	InputSystem*			m_inputSystem	= nullptr;
	AudioSystem*			m_audioSystem	= nullptr;
};

extern Engine* g_engine;

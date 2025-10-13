#pragma once

class RandomNumberGenerator;
class Renderer;
class InputSystem;

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
	RandomNumberGenerator*	m_rng			= nullptr;
};

extern Engine* g_engine;

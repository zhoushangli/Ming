#pragma once

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
	Renderer* m_renderer = nullptr;
	InputSystem* m_inputSystem = nullptr;
};

extern Engine* g_engine;

#pragma once

#include <Engine/Renderer/Renderer.hpp>

class Engine 
{
public:
	Engine();
	~Engine();

	/*void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();*/

	Renderer* GetRenderer() { return m_renderer; }

private:
	Renderer* m_renderer = nullptr;
};

extern Engine* g_engine;

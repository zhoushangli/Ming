#pragma once

class Renderer;
class RandomNumberGenerator;

class Engine 
{
public:
	Engine();
	~Engine();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	Renderer* m_renderer = nullptr;
	RandomNumberGenerator* m_rng;

private:

};

extern Engine* g_engine;

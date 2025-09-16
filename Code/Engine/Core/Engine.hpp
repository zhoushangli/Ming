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

	Renderer* GetRenderer() { return m_renderer; }
	RandomNumberGenerator* GetRNG() { return m_rng; }


private:
	Renderer* m_renderer = nullptr;
	RandomNumberGenerator* m_rng;
};

extern Engine* g_engine;

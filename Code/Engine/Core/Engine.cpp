#include "Engine.hpp"

Engine* g_engine = nullptr;

Engine::Engine()
{
	renderer = new Renderer();

	renderer->Startup();
}

Engine::~Engine()
{
	renderer->Shutdown();

	delete renderer;
	renderer = nullptr;
}

void Engine::BegineFrame()
{
	renderer->BeginFrame();
}

void Engine::EndFrame()
{
	renderer->EndFrame();
}
#include "Engine.hpp"

Engine* g_engine = nullptr;

Engine::Engine()
{
	m_renderer = new Renderer();
}

Engine::~Engine()
{
	delete m_renderer;
	m_renderer = nullptr;
}

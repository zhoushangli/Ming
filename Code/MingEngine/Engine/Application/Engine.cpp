#include "MingEngine/Engine/Application/Engine.hpp"

#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Math/RandomNumberGenerator.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Window/Window.hpp"

Engine* g_engine = nullptr;

Engine::Engine(EngineConfig config) : m_config(config)
{
	g_engine = this;

	if (config.m_eventSystemConfig.m_isEnable)
		m_eventSystem = new EventSystem(config.m_eventSystemConfig);
	if (config.m_windowConfig.m_isEnable)
		m_window = new Window(config.m_windowConfig);
	if (config.m_fileSystemConfig.m_isEnable)
		m_fileSystem = new FileSystem(config.m_fileSystemConfig);
	if (config.m_scriptSystemConfig.m_isEnabled)
		m_scriptSystem = new ScriptSystem(config.m_scriptSystemConfig);
	if (config.m_rendererConfig.m_isEnable)
		m_renderer = new Renderer(config.m_rendererConfig);
	if (config.m_inputConfig.m_isEnable)
		m_input = new InputSystem(config.m_inputConfig);
	if (config.m_audioConfig.m_isEnable)
		m_audio = new AudioSystem(config.m_audioConfig);
	if (config.m_imguiConfig.m_isEnable)
		m_imgui = new ImGuiSystem(config.m_imguiConfig);
}

Engine::~Engine()
{
	delete m_imgui;
	m_imgui = nullptr;

	delete m_audio;
	m_audio = nullptr;

	delete m_input;
	m_input = nullptr;

	delete m_renderer;
	m_renderer = nullptr;

	delete m_scriptSystem;
	m_scriptSystem = nullptr;

	delete m_fileSystem;
	m_fileSystem = nullptr;

	delete m_window;
	m_window = nullptr;

	delete m_eventSystem;
	m_eventSystem = nullptr;
}

void Engine::Startup()
{
	if (m_eventSystem != nullptr)
		m_eventSystem->Startup();
	if (m_window != nullptr)
		m_window->Startup();
	if (m_fileSystem != nullptr)
		m_fileSystem->Startup();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->Startup();
	if (m_renderer != nullptr)
		m_renderer->Startup();
	if (m_imgui != nullptr)
		m_imgui->Startup();
	if (m_input != nullptr)
		m_input->Startup();
	if (m_audio != nullptr)
		m_audio->Startup();
}

void Engine::Shutdown()
{
	if (m_audio != nullptr)
		m_audio->Shutdown();
	if (m_input != nullptr)
		m_input->Shutdown();
	if (m_imgui != nullptr)
		m_imgui->Shutdown();
	if (m_renderer != nullptr)
		m_renderer->Shutdown();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->Shutdown();
	if (m_fileSystem != nullptr)
		m_fileSystem->Shutdown();
	if (m_window != nullptr)
		m_window->Shutdown();
	if (m_eventSystem != nullptr)
		m_eventSystem->Shutdown();
}

void Engine::BeginFrame()
{
	if (m_eventSystem != nullptr)
		m_eventSystem->BeginFrame();
	if (m_window != nullptr)
		m_window->BeginFrame();
	if (m_fileSystem != nullptr)
		m_fileSystem->BeginFrame();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->BeginFrame();
	if (m_imgui != nullptr)
		m_imgui->BeginFrame();
	if (m_renderer != nullptr)
		m_renderer->BeginFrame();
	if (m_input != nullptr)
		m_input->BeginFrame();
	if (m_audio != nullptr)
		m_audio->BeginFrame();
}

void Engine::EndFrame()
{
	if (m_eventSystem != nullptr)
		m_eventSystem->EndFrame();
	if (m_window != nullptr)
		m_window->EndFrame();
	if (m_fileSystem != nullptr)
		m_fileSystem->EndFrame();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->EndFrame();
	if (m_imgui != nullptr)
		m_imgui->EndFrame();
	if (m_renderer != nullptr)
		m_renderer->EndFrame();
	if (m_input != nullptr)
		m_input->EndFrame();
	if (m_audio != nullptr)
		m_audio->EndFrame();
}

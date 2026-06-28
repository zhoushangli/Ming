#include "MingEngine/Engine/Application/Engine.hpp"

#include "MingEngine/Core/Math/RandomNumberGenerator.hpp"
#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

Engine* g_engine = nullptr;

Engine::Engine(EngineConfig config) : m_config(config)
{
	g_engine = this;

	if (config.m_eventSystemConfig.m_isEnable)
		m_eventSystem = new EventSystem(config.m_eventSystemConfig);
	if (config.m_windowConfig.m_isEnable)
		m_windowSystem = new WindowSystem(config.m_windowConfig);
	if (config.m_fileSystemConfig.m_isEnable)
		m_fileSystem = new FileSystem(config.m_fileSystemConfig);
	if (config.m_scriptSystemConfig.m_isEnabled)
		m_scriptSystem = new ScriptSystem(config.m_scriptSystemConfig);
	if (config.m_rendererConfig.m_isEnable)
		m_renderer = new Renderer(config.m_rendererConfig);
	if (config.m_inputConfig.m_isEnable)
		m_inputSystem = new InputSystem(config.m_inputConfig);
	if (config.m_audioConfig.m_isEnable)
		m_audioSystem = new AudioSystem(config.m_audioConfig);
	if (config.m_imguiConfig.m_isEnable)
		m_imguiSystem = new ImGuiSystem(config.m_imguiConfig);
}

Engine::~Engine()
{
	delete m_imguiSystem;
	m_imguiSystem = nullptr;

	delete m_audioSystem;
	m_audioSystem = nullptr;

	delete m_inputSystem;
	m_inputSystem = nullptr;

	delete m_renderer;
	m_renderer = nullptr;

	delete m_scriptSystem;
	m_scriptSystem = nullptr;

	delete m_fileSystem;
	m_fileSystem = nullptr;

	delete m_windowSystem;
	m_windowSystem = nullptr;

	delete m_eventSystem;
	m_eventSystem = nullptr;
}

void Engine::Startup()
{
	if (m_eventSystem != nullptr)
		m_eventSystem->Startup();
	if (m_windowSystem != nullptr)
		m_windowSystem->Startup();
	if (m_fileSystem != nullptr)
		m_fileSystem->Startup();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->Startup();
	if (m_renderer != nullptr)
		m_renderer->Startup();
	if (m_imguiSystem != nullptr)
		m_imguiSystem->Startup();
	if (m_inputSystem != nullptr)
		m_inputSystem->Startup();
	if (m_audioSystem != nullptr)
		m_audioSystem->Startup();
}

void Engine::Shutdown()
{
	if (m_audioSystem != nullptr)
		m_audioSystem->Shutdown();
	if (m_inputSystem != nullptr)
		m_inputSystem->Shutdown();
	if (m_imguiSystem != nullptr)
		m_imguiSystem->Shutdown();
	if (m_renderer != nullptr)
		m_renderer->Shutdown();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->Shutdown();
	if (m_fileSystem != nullptr)
		m_fileSystem->Shutdown();
	if (m_windowSystem != nullptr)
		m_windowSystem->Shutdown();
	if (m_eventSystem != nullptr)
		m_eventSystem->Shutdown();
}

void Engine::BeginFrame()
{
	if (m_eventSystem != nullptr)
		m_eventSystem->BeginFrame();
	if (m_windowSystem != nullptr)
		m_windowSystem->BeginFrame();
	if (m_fileSystem != nullptr)
		m_fileSystem->BeginFrame();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->BeginFrame();
	if (m_imguiSystem != nullptr)
		m_imguiSystem->BeginFrame();
	if (m_renderer != nullptr)
		m_renderer->BeginFrame();
	if (m_inputSystem != nullptr)
		m_inputSystem->BeginFrame();
	if (m_audioSystem != nullptr)
		m_audioSystem->BeginFrame();
}

void Engine::EndFrame()
{
	if (m_imguiSystem != nullptr)
		m_imguiSystem->EndFrame();
	if (m_renderer != nullptr)
		m_renderer->EndFrame();
	if (m_audioSystem != nullptr)
		m_audioSystem->EndFrame();
	if (m_inputSystem != nullptr)
		m_inputSystem->EndFrame();
	if (m_scriptSystem != nullptr)
		m_scriptSystem->EndFrame();
	if (m_fileSystem != nullptr)
		m_fileSystem->EndFrame();
	if (m_windowSystem != nullptr)
		m_windowSystem->EndFrame();
	if (m_eventSystem != nullptr)
		m_eventSystem->EndFrame();
}

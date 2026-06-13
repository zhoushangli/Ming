#pragma once

#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Console/DevConsole.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Window/Window.hpp"
#include "MingEngine/File/FileSystem.hpp"

class RandomNumberGenerator;
class Window;
class Renderer;
class InputSystem;
class AudioSystem;

struct EngineConfig
{
	EventSystemConfig m_eventSystemConfig;
	DevConsoleConfig  m_devConsoleConfig;
	WindowConfig      m_windowConfig;
	RendererConfig    m_rendererConfig;
	AudioConfig       m_audioConfig;
	InputConfig       m_inputConfig;
	ImGuiConfig       m_imguiConfig;
};

class Engine
{
public:
	Engine(EngineConfig config);
	~Engine();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

public:
	EngineConfig m_config;

	EventSystem* m_eventSystem = nullptr;
	DevConsole*  m_devConsole  = nullptr;
	Window*      m_window      = nullptr;
	FileSystem*  m_fileSystem  = nullptr;
	Renderer*    m_renderer    = nullptr;
	InputSystem* m_input       = nullptr;
	AudioSystem* m_audio       = nullptr;
	ImGuiSystem* m_imgui       = nullptr;
};

extern Engine* g_engine;

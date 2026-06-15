#pragma once

#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Window/Window.hpp"

class RandomNumberGenerator;
class Window;
class Renderer;
class InputSystem;
class AudioSystem;

struct EngineConfig
{
	EventSystemConfig  m_eventSystemConfig;
	WindowConfig       m_windowConfig;
	FileSystemConfig   m_fileSystemConfig;
	ScriptSystemConfig m_scriptSystemConfig;
	RendererConfig     m_rendererConfig;
	AudioConfig        m_audioConfig;
	InputConfig        m_inputConfig;
	ImGuiConfig        m_imguiConfig;
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

	EventSystem*  m_eventSystem  = nullptr;
	Window*       m_window       = nullptr;
	FileSystem*   m_fileSystem   = nullptr;
	ScriptSystem* m_scriptSystem = nullptr;
	Renderer*     m_renderer     = nullptr;
	InputSystem*  m_input        = nullptr;
	AudioSystem*  m_audio        = nullptr;
	ImGuiSystem*  m_imgui        = nullptr;
};

extern Engine* g_engine;


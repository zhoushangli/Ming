#pragma once

#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Engine/Network/NetworkSystem.hpp"

struct EngineConfig
{
	EventSystemConfig   m_eventSystemConfig;
	WindowSystemConfig  m_windowConfig;
	FileSystemConfig    m_fileSystemConfig;
	ScriptSystemConfig  m_scriptSystemConfig;
	RendererConfig      m_rendererConfig;
	AudioSystemConfig   m_audioConfig;
	InputSystemConfig   m_inputConfig;
	ImGuiSystemConfig   m_imguiConfig;
	NetworkSystemConfig m_networkConfig;
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

	EventSystem*   m_eventSystem   = nullptr;
	WindowSystem*  m_windowSystem  = nullptr;
	FileSystem*    m_fileSystem    = nullptr;
	ScriptSystem*  m_scriptSystem  = nullptr;
	Renderer*      m_renderer      = nullptr;
	InputSystem*   m_inputSystem   = nullptr;
	AudioSystem*   m_audioSystem   = nullptr;
	ImGuiSystem*   m_imguiSystem   = nullptr;
	NetworkSystem* m_networkSystem = nullptr;
};

extern Engine* g_engine;

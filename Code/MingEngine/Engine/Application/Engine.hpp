#pragma once

#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Scene/SceneSystem.hpp"
#include "MingEngine/Engine/Physics/PhysicsServer.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Network/NetworkSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

#include <functional>

class WorkerThreadPool;

struct EngineConfig
{
	WindowSystemConfig   m_windowConfig;
	FileSystemConfig     m_fileSystemConfig;
	ScriptSystemConfig   m_scriptSystemConfig;
	RendererServerConfig m_rendererServerConfig;
	AudioSystemConfig    m_audioConfig;
	InputSystemConfig    m_inputConfig;
	NetworkSystemConfig  m_networkConfig;
	bool                 m_enablePhysics        = false;
	bool                 m_enableWorkerThreadPool = false;
};

class Engine
{
public:
	Engine(EngineConfig config);
	~Engine();

	bool Startup();
	void Shutdown();
	void StopWorkerThreadPool();
	bool HasWorkerThreadPool() const { return m_workerThreadPool != nullptr; }
	bool Update(float deltaSeconds, std::function<bool()> const& shouldQuit = {});
	void Render() const;
	void BeginFrame();
	void EndFrame();

	void SetEditorMode(bool enabled) { m_isEditorMode = enabled; }

	bool IsEditorMode() const { return m_isEditorMode; }

public:
	EngineConfig m_config;

	WindowSystem*  m_windowSystem  = nullptr;
	FileSystem*    m_fileSystem    = nullptr;
	ScriptSystem*  m_scriptSystem  = nullptr;
	InputSystem*   m_inputSystem   = nullptr;
	AudioSystem*   m_audioSystem   = nullptr;
	NetworkSystem* m_networkSystem = nullptr;

	SceneSystem* m_sceneSystem = nullptr;
	PhysicsServer* m_physicsServer = nullptr;

	RenderServer* m_renderServer = nullptr;
	TextServer*   m_textServer   = nullptr;

private:
	WorkerThreadPool* m_workerThreadPool = nullptr;
	double m_physicsAccumulator = 0.0;
	bool   m_isEditorMode       = false;
};

extern Engine* g_engine;

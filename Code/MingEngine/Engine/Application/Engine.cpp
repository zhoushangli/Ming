#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include "MingEngine/Core/Math/RandomNumberGenerator.hpp"
#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

#include <cmath>

Engine* g_engine = nullptr;

Engine::Engine(EngineConfig config) : m_config(config)
{
	g_engine = this;
	m_sceneSystem = new SceneSystem();
	m_physicsServer = new PhysicsServer();

	if (config.m_eventSystemConfig.m_isEnable)
	{
		m_eventSystem = MemNew<EventSystem>(config.m_eventSystemConfig);
	}

	if (config.m_windowConfig.m_isEnable)
	{
		m_windowSystem = MemNew<WindowSystem>(config.m_windowConfig);
	}

	if (config.m_fileSystemConfig.m_isEnable)
	{
		m_fileSystem = MemNew<FileSystem>(config.m_fileSystemConfig);
	}

	if (config.m_scriptSystemConfig.m_isEnabled)
	{
		m_scriptSystem = MemNew<ScriptSystem>(config.m_scriptSystemConfig);
	}

	if (config.m_rendererServerConfig.m_isEnable)
	{
		m_renderServer = new RenderServer(config.m_rendererServerConfig);
	}

	m_textServer = new TextServer();
	if (config.m_inputConfig.m_isEnable)
	{
		m_inputSystem = MemNew<InputSystem>(config.m_inputConfig);
	}

	if (config.m_audioConfig.m_isEnable)
	{
		m_audioSystem = MemNew<AudioSystem>(config.m_audioConfig);
	}

	if (config.m_networkConfig.m_isEnable)
	{
		m_networkSystem = MemNew<NetworkSystem>(config.m_networkConfig);
	}
}

Engine::~Engine()
{
	delete m_sceneSystem;
	m_sceneSystem = nullptr;
	delete m_physicsServer;
	m_physicsServer = nullptr;

	ClassDatabase::ClearGlobalObjects();

	MemDelete(m_audioSystem);
	m_audioSystem = nullptr;

	MemDelete(m_inputSystem);
	m_inputSystem = nullptr;

	MemDelete(m_fileSystem);
	m_fileSystem = nullptr;

	delete m_renderServer;
	m_renderServer = nullptr;

	delete m_textServer;
	m_textServer = nullptr;

	MemDelete(m_scriptSystem);
	m_scriptSystem = nullptr;

	MemDelete(m_windowSystem);
	m_windowSystem = nullptr;

	MemDelete(m_eventSystem);
	m_eventSystem = nullptr;

	MemDelete(m_networkSystem);
	m_networkSystem = nullptr;
}

bool Engine::Startup()
{
	if (m_eventSystem != nullptr)
	{
		m_eventSystem->Startup();
	}

	if (m_windowSystem != nullptr)
	{
		m_windowSystem->Startup();
	}

	if (m_renderServer != nullptr)
	{
		m_renderServer->Startup();
	}

	if (m_fileSystem != nullptr)
	{
		m_fileSystem->Startup();
	}

	if (m_scriptSystem != nullptr)
	{
		m_scriptSystem->Startup();
	}

	if (m_inputSystem != nullptr)
	{
		m_inputSystem->Startup();
	}

	if (m_audioSystem != nullptr)
	{
		m_audioSystem->Startup();
	}

	if (m_networkSystem != nullptr)
	{
		m_networkSystem->Startup();
	}

	if (m_windowSystem != nullptr && m_renderServer != nullptr && m_inputSystem != nullptr && m_eventSystem != nullptr)
	{
		m_sceneSystem->Startup();
	}

	if (m_config.m_enablePhysics && !m_physicsServer->Startup())
	{
		ERR_PRINT("Cannot start the physics world.");
		return false;
	}

	return true;
}

void Engine::Shutdown()
{
	m_sceneSystem->Shutdown();
	m_physicsServer->Shutdown();
	m_physicsAccumulator = 0.0;

	if (m_networkSystem != nullptr)
	{
		m_networkSystem->Shutdown();
	}

	if (m_audioSystem != nullptr)
	{
		m_audioSystem->Shutdown();
	}

	if (m_inputSystem != nullptr)
	{
		m_inputSystem->Shutdown();
	}

	if (m_scriptSystem != nullptr)
	{
		m_scriptSystem->Shutdown();
	}

	if (m_renderServer != nullptr)
	{
		m_renderServer->Shutdown();
	}

	if (m_fileSystem != nullptr)
	{
		m_fileSystem->Shutdown();
	}

	if (m_windowSystem != nullptr)
	{
		m_windowSystem->Shutdown();
	}

	if (m_eventSystem != nullptr)
	{
		m_eventSystem->Shutdown();
	}
}

void Engine::BeginFrame()
{
	if (m_eventSystem != nullptr)
	{
		m_eventSystem->BeginFrame();
	}

	if (m_windowSystem != nullptr)
	{
		m_windowSystem->BeginFrame();
	}

	if (m_fileSystem != nullptr)
	{
		m_fileSystem->BeginFrame();
	}

	if (m_scriptSystem != nullptr)
	{
		m_scriptSystem->BeginFrame();
	}

	if (m_renderServer != nullptr)
	{
		m_renderServer->BeginFrame();
	}

	if (m_inputSystem != nullptr)
	{
		m_inputSystem->BeginFrame();
	}

	if (m_audioSystem != nullptr)
	{
		m_audioSystem->BeginFrame();
	}

	if (m_networkSystem != nullptr)
	{
		m_networkSystem->BeginFrame();
	}
}

void Engine::EndFrame()
{
	m_sceneSystem->EndFrame();

	if (m_networkSystem != nullptr)
	{
		m_networkSystem->EndFrame();
	}

	if (m_renderServer != nullptr)
	{
		m_renderServer->EndFrame();
	}

	if (m_audioSystem != nullptr)
	{
		m_audioSystem->EndFrame();
	}

	if (m_inputSystem != nullptr)
	{
		m_inputSystem->EndFrame();
	}

	if (m_scriptSystem != nullptr)
	{
		m_scriptSystem->EndFrame();
	}

	if (m_fileSystem != nullptr)
	{
		m_fileSystem->EndFrame();
	}

	if (m_windowSystem != nullptr)
	{
		m_windowSystem->EndFrame();
	}

	if (m_eventSystem != nullptr)
	{
		m_eventSystem->EndFrame();
	}
}

bool Engine::Update(float deltaSeconds, std::function<bool()> const& shouldQuit)
{
	if (m_config.m_enablePhysics && std::isfinite(deltaSeconds) && deltaSeconds >= 0.f
		&& (!shouldQuit || !shouldQuit()))
	{
		constexpr double fixedDelta = 1.0 / 60.0;
		constexpr int maxSteps = 8;
		m_physicsAccumulator += static_cast<double>(deltaSeconds);

		int steps = 0;
		while (m_physicsAccumulator >= fixedDelta && steps < maxSteps)
		{
			m_sceneSystem->PhysicsProcess(static_cast<float>(fixedDelta));
			if (!m_physicsServer->Step(static_cast<float>(fixedDelta)))
			{
				ERR_PRINT("Physics simulation failed; stopping the game.");
				return false;
			}

			m_physicsAccumulator -= fixedDelta;
			++steps;
			if (shouldQuit && shouldQuit())
			{
				break;
			}
		}

		if (steps == maxSteps && m_physicsAccumulator >= fixedDelta)
		{
			m_physicsAccumulator = std::fmod(m_physicsAccumulator, fixedDelta);
		}
	}

	m_sceneSystem->Process(deltaSeconds);
	return true;
}

void Engine::Render() const
{
	if (m_renderServer != nullptr)
	{
		m_renderServer->Render();
	}
}

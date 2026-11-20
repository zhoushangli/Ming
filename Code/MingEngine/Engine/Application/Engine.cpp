#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include "MingEngine/Core/Math/RandomNumberGenerator.hpp"
#include "MingEngine/Core/OS/WorkerThreadPool.hpp"
#include "MingEngine/Engine/Audio/AudioSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

Engine* g_engine = nullptr;

Engine::Engine(EngineConfig config) : m_config(config)
{
	g_engine = this;
	m_sceneSystem = MemNew<SceneSystem>();
	m_physicsServer = MemNew<PhysicsServer>();

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
		m_renderServer = MemNew<RenderServer>(config.m_rendererServerConfig);
	}

	m_textServer = MemNew<TextServer>();
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
	StopWorkerThreadPool();

	MemDelete(m_sceneSystem);
	m_sceneSystem = nullptr;
	MemDelete(m_physicsServer);
	m_physicsServer = nullptr;

	ClassDatabase::ClearGlobalObjects();

	MemDelete(m_audioSystem);
	m_audioSystem = nullptr;

	MemDelete(m_inputSystem);
	m_inputSystem = nullptr;

	MemDelete(m_fileSystem);
	m_fileSystem = nullptr;

	MemDelete(m_renderServer);
	m_renderServer = nullptr;

	MemDelete(m_textServer);
	m_textServer = nullptr;

	MemDelete(m_scriptSystem);
	m_scriptSystem = nullptr;

	MemDelete(m_windowSystem);
	m_windowSystem = nullptr;

	MemDelete(m_networkSystem);
	m_networkSystem = nullptr;
}

bool Engine::Startup()
{
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

	if (m_config.m_enableWorkerThreadPool && m_scriptSystem != nullptr)
	{
		m_workerThreadPool = MemNew<WorkerThreadPool>();
		int const threadCount = static_cast<int>((std::max)(1u, std::thread::hardware_concurrency()));
		if (!m_workerThreadPool->Start(threadCount))
		{
			StopWorkerThreadPool();
			ERR_PRINT("Cannot start the worker thread pool.");
			return false;
		}

		ClassDatabase::RegisterGlobalObject(m_workerThreadPool);
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

	if (m_windowSystem != nullptr && m_renderServer != nullptr && m_inputSystem != nullptr)
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

void Engine::StopWorkerThreadPool()
{
	if (m_workerThreadPool == nullptr)
	{
		return;
	}

	m_workerThreadPool->Stop();
	ClassDatabase::UnregisterGlobalObject(WorkerThreadPool::GetStaticClassName());
	MemDelete(m_workerThreadPool);
	m_workerThreadPool = nullptr;
}

void Engine::Shutdown()
{
	StopWorkerThreadPool();

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
}

void Engine::BeginFrame()
{
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

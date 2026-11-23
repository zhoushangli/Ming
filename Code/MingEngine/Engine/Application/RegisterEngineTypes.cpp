#include "MingEngine/Engine/Application/RegisterEngineTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/OS/WorkerThreadPool.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Application/ProjectSettingsFormat.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

#pragma region Engine

void RegisterEngineTypes()
{
	ClassDatabase::SetApiType(ApiType::Runtime);

	ClassDatabase::RegisterClass<ProjectSettingsLoader>();
	ClassDatabase::RegisterClass<ProjectSettingsSaver>();
	ClassDatabase::RegisterClass<ProjectSettings>();
	ResourceLoader::AddLoader(CreateRef<ProjectSettingsLoader>());
	ResourceSaver::AddSaver(CreateRef<ProjectSettingsSaver>());

	// Engine system types
	ClassDatabase::RegisterClass<SystemBase>(true);
	ClassDatabase::RegisterAbstractClass<WindowSystem>();
	ClassDatabase::RegisterAbstractClass<RenderServer>();

	ClassDatabase::RegisterSingletonClass<WorkerThreadPool>();
	ClassDatabase::RegisterSingletonClass<InputSystem>();
	ClassDatabase::RegisterSingletonClass<FileSystem>();

	// 1) Register the available singleton instances.
	if (g_engine != nullptr && g_engine->m_workerThreadPool != nullptr)
	{
		ClassDatabase::RegisterSingleton(g_engine->m_workerThreadPool);
	}

	if (g_engine != nullptr && g_engine->m_inputSystem != nullptr)
	{
		ClassDatabase::RegisterSingleton(g_engine->m_inputSystem);
	}

	if (g_engine != nullptr && g_engine->m_fileSystem != nullptr)
	{
		ClassDatabase::RegisterSingleton(g_engine->m_fileSystem);
	}
}

#pragma endregion

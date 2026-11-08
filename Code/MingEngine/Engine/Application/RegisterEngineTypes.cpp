#include "MingEngine/Engine/Application/RegisterEngineTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Application/ProjectSettingsFormat.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"

#pragma region Engine

void RegisterEngineTypes()
{
	ClassDatabase::SetApiType(ApiType::Runtime);

	ClassDatabase::RegisterClass<ProjectSettingsLoader>();
	ClassDatabase::RegisterClass<ProjectSettingsSaver>();
	ResourceLoader::AddLoader(CreateRef<ProjectSettingsLoader>());
	ResourceSaver::AddSaver(CreateRef<ProjectSettingsSaver>());
	ClassDatabase::RegisterClass<ProjectSettings>();

	// Engine system types
	ClassDatabase::RegisterClass<SystemBase>(true);
	ClassDatabase::RegisterSingletonClass<InputSystem>();
	ClassDatabase::RegisterSingletonClass<FileSystem>();

	// Global objects
	if (g_engine != nullptr && g_engine->m_inputSystem != nullptr)
	{
		ClassDatabase::RegisterGlobalObject(g_engine->m_inputSystem);
	}

	if (g_engine != nullptr && g_engine->m_fileSystem != nullptr)
	{
		ClassDatabase::RegisterGlobalObject(g_engine->m_fileSystem);
	}
}

#pragma endregion

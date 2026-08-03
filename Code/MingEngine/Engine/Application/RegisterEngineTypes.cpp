#include "MingEngine/Engine/Application/RegisterEngineTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Application/ProjectSettingsFormat.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"

namespace
{
ProjectSettingsLoader* projectSettingsLoader = new ProjectSettingsLoader();
ProjectSettingsSaver*  projectSettingsSaver  = new ProjectSettingsSaver();
} // namespace

#pragma region Engine

void RegisterEngineTypes()
{
	ClassDatabase::SetApiType(ApiType::Runtime);

	ClassDatabase::RegisterClass<ProjectSettingsLoader>();
	ClassDatabase::RegisterClass<ProjectSettingsSaver>();
	ResourceLoader::AddLoader(Ref<ProjectSettingsLoader>(projectSettingsLoader));
	ResourceSaver::AddSaver(Ref<ProjectSettingsSaver>(projectSettingsSaver));
	ClassDatabase::RegisterClass<ProjectSettings>();

	// Engine system types
	ClassDatabase::RegisterClass<SystemBase>(true);
	ClassDatabase::RegisterClass<InputSystem>();

	// Global objects
	// ClassDatabase::RegisterGlobalObject(g_engine->m_inputSystem);
}

#pragma endregion

#include "MingEngine/Engine/Application/ProjectSettings.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

#include <utility>

Ref<ProjectSettings> ProjectSettings::s_instance = Ref<ProjectSettings>(nullptr);

Ref<ProjectSettings> ProjectSettings::Get()
{
	if (s_instance == nullptr)
	{
		s_instance = ResourceLoader::Load(GetSettingsPath());
		if (!s_instance.IsValid())
		{
			s_instance = CreateRef<ProjectSettings>();
			ResourceSaver::Save(GetSettingsPath(), s_instance);
		}
	}

	return s_instance;
}

bool ProjectSettings::CopyFrom(Resource&& other)
{
	ProjectSettings* otherSettings = dynamic_cast<ProjectSettings*>(&other);
	if (otherSettings == nullptr)
	{
		return false;
	}

	MoveBaseFrom(std::move(other));
	m_startScenePath = std::move(otherSettings->m_startScenePath);

	return true;
}

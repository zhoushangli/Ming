#include "MingEngine/Engine/Application/ProjectSettings.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

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

bool ProjectSettings::CopyFrom(Resource const& other)
{
	ProjectSettings const* otherSettings = dynamic_cast<ProjectSettings const*>(&other);
	if (otherSettings == nullptr)
	{
		return false;
	}

	m_startScenePath = otherSettings->m_startScenePath;

	return true;
}

#pragma once

#include "MingEngine/Core/Object/Resource.hpp"

#include <string>

class ProjectSettings : public Resource
{
	MCLASS(ProjectSettings, Resource)

public:
	static Ref<ProjectSettings>  Get();
	static constexpr char const* GetSettingsPath() { return "res://project.ming"; }

	bool CopyFrom(Resource&& other) override;

private:
	static void BindMethods() {}

public:
	VirtualPath m_startScenePath;

private:
	static Ref<ProjectSettings> s_instance;
};

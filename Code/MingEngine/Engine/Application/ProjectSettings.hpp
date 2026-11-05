#pragma once

#include "MingEngine/Core/Object/Resource.hpp"

#include <string>
#include <filesystem>
#include <vector>

class ProjectSettings : public Resource
{
	MCLASS(ProjectSettings, Resource)

public:
	struct AutoLoad
	{
		std::string m_name;
		VirtualPath m_path;
	};

public:
	static Ref<ProjectSettings> Get();
	static bool                 Load(std::filesystem::path const& directory, std::string& error);
	static Ref<ProjectSettings> Parse(std::string const& text, std::string& error);
	static Ref<ProjectSettings> Read(std::filesystem::path const& directory, std::string& error);
	static void                 Reset();

	static constexpr char const* GetSettingsPath() { return "res://project.ming"; }

	bool CopyFrom(Resource&& other) override;

private:
	static void BindMethods() {}

public:
	VirtualPath m_startScenePath;
	std::vector<AutoLoad> m_autoLoads;

private:
	static Ref<ProjectSettings> s_instance;
};

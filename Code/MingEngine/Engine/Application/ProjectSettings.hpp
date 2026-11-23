#pragma once

#include "MingEngine/Core/IO/Resource.hpp"

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

	static bool ParseMainScenePath(std::string const& text, VirtualPath& path, std::string& error);
	static bool ValidateAutoloads(std::vector<AutoLoad> const& entries, std::string& error);

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

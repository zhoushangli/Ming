#include "MingEngine/Engine/Application/ProjectSettings.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

#include <utility>
#include <fstream>
#include <unordered_set>
#include "ThirdParty/nlohmann/json.hpp"

Ref<ProjectSettings> ProjectSettings::s_instance = Ref<ProjectSettings>(nullptr);

Ref<ProjectSettings> ProjectSettings::Get()
{
	return s_instance;
}

bool ProjectSettings::Load(std::filesystem::path const& directory, std::string& error)
{
	Ref<ProjectSettings> settings = Read(directory, error);
	if (!settings.IsValid())
	{
		return false;
	}
	settings->SetVirtualPath(GetSettingsPath());
	s_instance = settings;
	return true;
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
	m_autoLoads = std::move(otherSettings->m_autoLoads);

	return true;
}

void ProjectSettings::Reset()
{
	s_instance = nullptr;
}

Ref<ProjectSettings> ProjectSettings::Parse(std::string const& text, std::string& error)
{
	error.clear();
	auto root = nlohmann::json::parse(text, nullptr, false);
	if (!root.is_object() || root.value("type", nlohmann::json()) != "ProjectSettings" || !root.contains("version")
		|| !root["version"].is_number_unsigned() || !root.contains("start_scene_path")
		|| !root["start_scene_path"].is_string())
	{
		error = "Invalid project.ming contents.";
		return {};
	}
	if (root["version"] != 1)
	{
		error = "Unsupported project.ming version.";
		return {};
	}

	Ref<ProjectSettings> settings = CreateRef<ProjectSettings>();
	std::string const    path     = root["start_scene_path"].get<std::string>();
	if (!path.empty()
		&& (!VirtualPath::TryParse(path, settings->m_startScenePath)
			|| !settings->m_startScenePath.HasExtension(".tscn")))
	{
		error = "The project main scene must be a res:// path ending in .tscn.";
		return {};
	}
	if (root.contains("autoloads"))
	{
		if (!root["autoloads"].is_array())
		{
			error = "Project autoloads must be an ordered array.";
			return {};
		}

		std::unordered_set<std::string> names;
		for (auto const& entry : root["autoloads"])
		{
			if (!entry.is_object() || !entry.contains("name") || !entry["name"].is_string()
				|| !entry.contains("path") || !entry["path"].is_string())
			{
				error = "Each autoload requires a name and resource path.";
				return {};
			}

			AutoLoad autoload;
			autoload.m_name = entry["name"].get<std::string>();
			if (autoload.m_name.empty() || autoload.m_name == "." || autoload.m_name == ".."
				|| autoload.m_name.find_first_of("/\\:") != std::string::npos || !names.insert(autoload.m_name).second
				|| !VirtualPath::TryParse(entry["path"].get<std::string>(), autoload.m_path)
				|| (!autoload.m_path.HasExtension(".tscn") && !autoload.m_path.HasExtension(".cs")))
			{
				error = "Invalid or duplicate autoload: " + autoload.m_name;
				return {};
			}

			settings->m_autoLoads.push_back(std::move(autoload));
		}
	}

	return settings;
}

Ref<ProjectSettings> ProjectSettings::Read(std::filesystem::path const& directory, std::string& error)
{
	std::ifstream input(directory / L"project.ming", std::ios::binary);
	if (!input)
	{
		error = "Cannot read project.ming.";
		return {};
	}
	std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	return Parse(text, error);
}

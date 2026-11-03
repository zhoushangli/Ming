#include "MingEngine/Engine/Application/ProjectSettingsFormat.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include <cstdint>

namespace
{
using Json = nlohmann::ordered_json;

constexpr char const* kProjectSettingsExtension = ".ming";
constexpr uint32_t    kProjectSettingsVersion   = 1;

} // namespace

std::vector<std::string> ProjectSettingsLoader::GetSupportedExtensions() const
{
	return { kProjectSettingsExtension };
}

Ref<Resource> ProjectSettingsLoader::Load(VirtualPath const& virtualPath)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return Ref<Resource>();
	}

	std::string text;
	if (!g_engine->m_fileSystem->ReadText(virtualPath, text))
	{
		return Ref<Resource>();
	}

	std::string          error;
	Ref<ProjectSettings> settings = ProjectSettings::Parse(text, error);
	if (!settings.IsValid())
	{
		ERR_PRINT(error);
		return {};
	}

	settings->SetVirtualPath(virtualPath);
	return settings;
}

bool ProjectSettingsSaver::CanSave(VirtualPath const& virtualPath, Variant const& value) const
{
	Ref<ProjectSettings> settings(value);
	return settings.IsValid() && virtualPath.HasExtension(kProjectSettingsExtension);
}

bool ProjectSettingsSaver::Save(VirtualPath const& virtualPath, Variant const& value)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr || !virtualPath.IsValid())
	{
		return false;
	}

	Ref<ProjectSettings> settings(value);
	if (!settings.IsValid())
	{
		return false;
	}

	Json root;
	root["type"]             = "ProjectSettings";
	root["version"]          = kProjectSettingsVersion;
	root["start_scene_path"] = settings->m_startScenePath.GetString();

	if (!g_engine->m_fileSystem->WriteText(virtualPath, root.dump(1, '\t')))
	{
		return false;
	}

	settings->SetVirtualPath(virtualPath);
	return true;
}

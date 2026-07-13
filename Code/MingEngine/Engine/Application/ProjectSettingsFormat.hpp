#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

class ProjectSettingsLoader : public ResourceFormatLoader
{
	MCLASS(ProjectSettingsLoader, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource>            Load(std::string const& virtualPath) override;

protected:
	static void BindMethods() {}
};

class ProjectSettingsSaver : public ResourceFormatSaver
{
	MCLASS(ProjectSettingsSaver, ResourceFormatSaver)

public:
	bool CanSave(std::string const& virtualPath, Variant const& value) const override;
	bool Save(std::string const& virtualPath, Variant const& value) override;

protected:
	static void BindMethods() {}
};

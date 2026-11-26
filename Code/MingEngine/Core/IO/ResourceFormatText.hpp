#pragma once

#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"

class ResourceFormatLoaderText : public ResourceFormatLoader
{
	MCLASS(ResourceFormatLoaderText, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource> Load(VirtualPath const& path) override;
	static Ref<Resource> Parse(std::string const& text, VirtualPath const& path, std::string& error);

protected:
	static void BindMethods() {}
};

class ResourceFormatSaverText : public ResourceFormatSaver
{
	MCLASS(ResourceFormatSaverText, ResourceFormatSaver)

public:
	bool CanSave(VirtualPath const& path, Variant const& value) const override;
	bool Save(VirtualPath const& path, Variant const& value) override;
	static bool Write(Ref<Resource> const& resource, std::string& text, std::string& error);

protected:
	static void BindMethods() {}
};

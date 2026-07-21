#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

class TextureResourceLoader : public ResourceFormatLoader
{
	MCLASS(TextureResourceLoader, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource>            Load(VirtualPath const& virtualPath) override;

protected:
	static void BindMethods() {};
};

class TextureResourceSaver : public ResourceFormatSaver
{
	MCLASS(TextureResourceSaver, ResourceFormatSaver)

public:
	bool CanSave(VirtualPath const& virtualPath, Variant const& value) const override;
	bool Save(VirtualPath const& virtualPath, Variant const& value) override;

protected:
	static void BindMethods() {};
};

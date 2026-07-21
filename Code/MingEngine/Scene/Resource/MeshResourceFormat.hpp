#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

class MeshResourceLoader : public ResourceFormatLoader
{
	MCLASS(MeshResourceLoader, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource>            Load(VirtualPath const& virtualPath) override;

protected:
	static void BindMethods() {};
};

class MeshResourceSaver : public ResourceFormatSaver
{
	MCLASS(MeshResourceSaver, ResourceFormatSaver)

public:
	bool CanSave(VirtualPath const& virtualPath, Variant const& value) const override;
	bool Save(VirtualPath const& virtualPath, Variant const& value) override;

protected:
	static void BindMethods() {};
};

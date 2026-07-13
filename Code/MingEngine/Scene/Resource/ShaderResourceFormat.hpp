#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

class MeshResourceLoader : public ResourceFormatLoader
{
	MCLASS(MeshResourceLoader, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource>            Load(std::string const& virtualPath) override;

protected:
	static void BindMethods() {};
};

class MeshResourceSaver : public ResourceFormatSaver
{
	MCLASS(MeshResourceSaver, ResourceFormatSaver)

public:
	bool CanSave(std::string const& virtualPath, Variant const& value) const override;
	bool Save(std::string const& virtualPath, Variant const& value) override;

protected:
	static void BindMethods() {};
};

#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

class PackedSceneLoader : public ResourceFormatLoader
{
	MCLASS(PackedSceneLoader, ResourceFormatLoader)

public:
	virtual std::vector<std::string> GetSupportedExtensions() const override;
	virtual Ref<Resource>            Load(VirtualPath const& virtualPath) override;

protected:
	static void BindMethods() {};
};

class PackedSceneSaver : public ResourceFormatSaver
{
	MCLASS(PackedSceneSaver, ResourceFormatSaver)

public:
	virtual bool CanSave(VirtualPath const& virtualPath, Variant const& value) const override;
	virtual bool Save(VirtualPath const& virtualPath, Variant const& value) override;

protected:
	static void BindMethods() {};
};

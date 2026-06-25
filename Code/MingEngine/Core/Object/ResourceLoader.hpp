#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"

#include <string>
#include <unordered_map>

class ResourceFormatLoader : public RefCounted
{
	MCLASS(ResourceFormatLoader, RefCounted)

public:
	virtual std::vector<std::string> GetSupportedExtensions() const = 0;
	virtual Ref<Resource>            Load(const std::string& virtualPath)  = 0;

	bool CanLoad(const std::string& path) const;

protected:
	static void BindMethods() {};
};

class ScriptLoader : public ResourceFormatLoader
{
	MCLASS(ScriptLoader, ResourceFormatLoader)

public:
	virtual std::vector<std::string> GetSupportedExtensions() const override;
	virtual Ref<Resource>            Load(const std::string& path) override;

protected:
	static void BindMethods() {};
};

class ResourceLoader
{
public:
	ResourceLoader();
	virtual ~ResourceLoader();

	static void          AddLoader(Ref<ResourceFormatLoader> loader);
	static Ref<Resource> Load(const std::string& virtualPath);

private:
	static constexpr int MaxLoaders = 64;

	static int                                            s_loaderCount;
	static Ref<ResourceFormatLoader>                      s_loader[MaxLoaders];
	static std::unordered_map<std::string, Ref<Resource>> s_loadedResources;
};
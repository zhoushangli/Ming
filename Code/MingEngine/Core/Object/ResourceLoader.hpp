#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class ResourceFormatLoader : public RefCounted
{
	MCLASS(ResourceFormatLoader, RefCounted)

public:
	virtual std::vector<std::string> GetSupportedExtensions() const       = 0;
	virtual Ref<Resource>            Load(VirtualPath const& virtualPath) = 0;

	bool CanLoad(VirtualPath const& path) const;

protected:
	static void BindMethods() {};
};

class ResourceLoader
{
public:
	ResourceLoader();
	virtual ~ResourceLoader();

	static void Startup() {}
	static void Shutdown() { s_loadedResources.clear(); }

	static void AddLoader(Ref<ResourceFormatLoader> loader);
	static bool CanLoad(VirtualPath const& virtualPath);
	// Load will return a cached resource if it has already been loaded
	static Ref<Resource> Load(VirtualPath const& virtualPath);
	// Load a resource from disk bypassing s_loadedResources.
	// Used by Reload() to obtain fresh data that will be moved into the existing
	// cached object via MoveFrom(), so that all Ref<> holders see the update.
	static Ref<Resource> LoadUncached(VirtualPath const& virtualPath);
	// Reload will force a resource to be reloaded, replacing the cached resource if it exists
	static Ref<Resource> Reload(VirtualPath const& virtualPath);

private:
	static constexpr int MaxLoaders = 64;

	static int                                            s_loaderCount;
	static Ref<ResourceFormatLoader>                      s_loader[MaxLoaders];
	static std::unordered_map<VirtualPath, Ref<Resource>> s_loadedResources;

	// Raw loading path shared by Load() and LoadUncached().
	// 1) Resolve import chain
	// 2) Iterate registered loaders and call loader->Load(path)
	static Ref<Resource> LoadInternal(VirtualPath const& virtualPath);
};

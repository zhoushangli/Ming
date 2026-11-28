#include "MingEngine/Core/IO/ResourceLoader.hpp"

#include "MingEngine/Core/IO/ResourceImporter.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include <utility>
#include <set>

namespace
{
thread_local std::set<std::string> s_loadingPaths;
}

int                                            ResourceLoader::s_loaderCount = 0;
Ref<ResourceFormatLoader>                      ResourceLoader::s_loader[MaxLoaders];
std::unordered_map<VirtualPath, Ref<Resource>> ResourceLoader::s_loadedResources;

void ResourceLoader::AddLoader(Ref<ResourceFormatLoader> loader)
{
	if (loader.IsValid() && s_loaderCount < MaxLoaders)
	{
		s_loader[s_loaderCount++] = loader;
	}
}

bool ResourceLoader::CanLoad(VirtualPath const& virtualPath)
{
	if (!virtualPath.IsValid())
	{
		return false;
	}

	for (int i = 0; i < s_loaderCount; ++i)
	{
		if (s_loader[i]->CanLoad(virtualPath))
		{
			return true;
		}
	}

	return false;
}

// Raw loading path shared by Load() and LoadUncached().
// 1) Resolve import chain
// 2) Iterate registered loaders and call loader->Load(path)
Ref<Resource> ResourceLoader::LoadInternal(VirtualPath const& virtualPath)
{
	auto& loading = s_loadingPaths;
	if (!loading.insert(virtualPath.GetString()).second)
	{
		ERR_PRINT("Circular resource dependency: " + virtualPath.GetString());
		return {};
	}

	struct LoadingGuard
	{
		std::set<std::string>& m_paths;
		std::string m_path;
		~LoadingGuard() { m_paths.erase(m_path); }
	} guard{ loading, virtualPath.GetString() };

	if (!virtualPath.IsValid())
	{
		return Ref<Resource>();
	}

	// If the resource is not an internal cache file
	// check if it has an import config and redirect to the imported file.
	if (!ResourceImporter::IsInternalResourcePath(virtualPath) && !ResourceImporter::IsImportConfigPath(virtualPath))
	{
		VirtualPath importPath;
		if (ResourceImporter::TryGetImportFile(virtualPath, importPath) && importPath != virtualPath)
		{
			Ref<Resource> resource = LoadInternal(importPath);
			if (resource.IsValid())
			{
				// Keep the user-selected source path so editor UI can show Pawn.obj instead of the imported cache file.
				resource->SetSourceFilePath(virtualPath);
			}

			return resource;
		}
	}

	// If the resource is an internal cache file or has no import config
	// load it directly from disk.
	for (int i = 0; i < s_loaderCount; ++i)
	{
		if (s_loader[i]->CanLoad(virtualPath))
		{
			Ref<Resource> resource = s_loader[i]->Load(virtualPath);
			return resource;
		}
	}

	return Ref<Resource>();
}

Ref<Resource> ResourceLoader::Load(VirtualPath const& virtualPath)
{
	if (s_loadingPaths.count(virtualPath.GetString()) != 0)
	{
		ERR_PRINT("Circular resource dependency: " + virtualPath.GetString());
		return {};
	}

	// 1) Return cached resource if already loaded
	auto const iter = s_loadedResources.find(virtualPath);
	if (iter != s_loadedResources.end())
	{
		return iter->second;
	}

	// 2) Load from disk and cache
	Ref<Resource> resource = LoadInternal(virtualPath);
	if (resource.IsValid())
	{
		s_loadedResources[virtualPath] = resource;
	}

	return resource;
}

Ref<Resource> ResourceLoader::LoadUncached(VirtualPath const& virtualPath)
{
	return LoadInternal(virtualPath);
}

Ref<Resource> ResourceLoader::Reload(VirtualPath const& virtualPath)
{
	// 1) Look up existing cached resource
	auto const    iter           = s_loadedResources.find(virtualPath);
	Ref<Resource> cachedResource = (iter != s_loadedResources.end()) ? iter->second : Ref<Resource>();

	// 2) Load fresh data from disk, bypassing cache
	Ref<Resource> freshResource = LoadUncached(virtualPath);
	if (!freshResource.IsValid())
	{
		return cachedResource;
	}

	// 3) Move fresh data into cached object so existing Ref<> holders see the update
	if (cachedResource.IsValid())
	{
		if (!cachedResource->MoveFrom(std::move(*freshResource)))
		{
			return cachedResource;
		}

		return cachedResource;
	}

	// 4) First-time load: insert into cache
	s_loadedResources[virtualPath] = freshResource;
	return freshResource;
}

void ResourceLoader::ReloadIfCached(VirtualPath const& virtualPath)
{
	VirtualPath output = virtualPath;
	VirtualPath mapped;
	if (ResourceImporter::TryGetImportFile(virtualPath, mapped))
	{
		output = mapped;
	}
	std::vector<VirtualPath> keys;
	for (auto const& entry : s_loadedResources)
	{
		VirtualPath resolved = entry.first;
		if (ResourceImporter::TryGetImportFile(entry.first, mapped))
		{
			resolved = mapped;
		}
		if (entry.first == virtualPath || entry.first == output || resolved == virtualPath || resolved == output)
		{
			keys.push_back(entry.first);
		}
	}

	for (auto const& key : keys)
	{
		Reload(key);
	}
}

bool ResourceFormatLoader::CanLoad(VirtualPath const& virtualPath) const
{
	std::vector<std::string> supportedExtensions = GetSupportedExtensions();

	for (const auto& ext : supportedExtensions)
	{
		if (virtualPath.HasExtension(ext))
		{
			return true;
		}
	}

	return false;
}

Ref<Resource> ResourceLoader::LoadForScript(String const& path)
{
	return Load(VirtualPath(path.ToUtf8()));
}

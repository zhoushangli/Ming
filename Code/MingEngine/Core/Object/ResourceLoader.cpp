#include "MingEngine/Core/Object/ResourceLoader.hpp"

#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

int                                            ResourceLoader::s_loaderCount = 0;
Ref<ResourceFormatLoader>                      ResourceLoader::s_loader[MaxLoaders];
std::unordered_map<std::string, Ref<Resource>> ResourceLoader::s_loadedResources;

void ResourceLoader::AddLoader(Ref<ResourceFormatLoader> loader)
{
	if (loader.IsValid() && s_loaderCount < MaxLoaders)
	{
		s_loader[s_loaderCount++] = loader;
	}
}

bool ResourceLoader::CanLoad(std::string const& virtualPath)
{
	if (!FileSystem::IsVirtualPath(virtualPath))
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
Ref<Resource> ResourceLoader::LoadInternal(std::string const& virtualPath)
{
	if (!FileSystem::IsVirtualPath(virtualPath))
	{
		return Ref<Resource>();
	}

	std::string const normalizedPath = virtualPath;

	if (!ResourceImporter::IsInternalResourcePath(normalizedPath)
		&& !ResourceImporter::IsImportConfigPath(normalizedPath))
	{
		std::string importPath;
		if (ResourceImporter::TryGetImportFile(normalizedPath, importPath) && importPath != normalizedPath)
		{
			Ref<Resource> resource = LoadInternal(importPath);
			if (resource.IsValid())
			{
				// Keep the user-selected source path so editor UI can show Pawn.obj instead of the imported cache file.
				resource->SetSourceFilePath(normalizedPath);
			}
			return resource;
		}
	}

	for (int i = 0; i < s_loaderCount; ++i)
	{
		if (s_loader[i]->CanLoad(normalizedPath))
		{
			Ref<Resource> resource = s_loader[i]->Load(normalizedPath);
			return resource;
		}
	}

	return Ref<Resource>();
}

Ref<Resource> ResourceLoader::Load(const std::string& virtualPath)
{
	std::string const normalizedPath = virtualPath;

	// 1) Return cached resource if already loaded
	auto const iter = s_loadedResources.find(normalizedPath);
	if (iter != s_loadedResources.end())
	{
		return iter->second;
	}

	// 2) Load from disk and cache
	Ref<Resource> resource = LoadInternal(normalizedPath);
	if (resource.IsValid())
	{
		s_loadedResources[normalizedPath] = resource;
	}
	return resource;
}

Ref<Resource> ResourceLoader::LoadUncached(std::string const& virtualPath) { return LoadInternal(virtualPath); }

Ref<Resource> ResourceLoader::Reload(const std::string& virtualPath)
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

	// 3) Copy fresh data into cached object so existing Ref<> holders see the update
	if (cachedResource.IsValid())
	{
		cachedResource->CopyFrom(*freshResource);
		return cachedResource;
	}

	// 4) First-time load: insert into cache
	s_loadedResources[virtualPath] = freshResource;
	return freshResource;
}

bool ResourceFormatLoader::CanLoad(const std::string& virtualPath) const
{
	std::vector<std::string> supportedExtensions = GetSupportedExtensions();

	for (const auto& ext : supportedExtensions)
	{
		if (virtualPath.size() >= ext.size()
			&& virtualPath.compare(virtualPath.size() - ext.size(), ext.size(), ext) == 0)
		{
			return true;
		}
	}
	return false;
}

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

Ref<Resource> ResourceLoader::Load(const std::string& virtualPath)
{
	if (!FileSystem::IsVirtualPath(virtualPath))
	{
		return Ref<Resource>();
	}

	std::string const normalizedPath = virtualPath;
	if (s_loadedResources.find(normalizedPath) != s_loadedResources.end())
	{
		return s_loadedResources[normalizedPath];
	}

	if (!ResourceImporter::IsInternalResourcePath(normalizedPath)
		&& !ResourceImporter::IsImportMetadataPath(normalizedPath))
	{
		std::string importPath;
		if (ResourceImporter::TryReadImportFile(normalizedPath, importPath) && importPath != normalizedPath)
		{
			Ref<Resource> resource = Load(importPath);
			if (resource.IsValid())
			{
				s_loadedResources[normalizedPath] = resource;
			}
			return resource;
		}
	}

	for (int i = 0; i < s_loaderCount; ++i)
	{
		if (s_loader[i]->CanLoad(normalizedPath))
		{
			Ref<Resource> resource = s_loader[i]->Load(normalizedPath);
			if (resource.IsValid())
			{
				s_loadedResources[normalizedPath] = resource;
			}
			return resource;
		}
	}

	return Ref<Resource>();
}

bool ResourceFormatLoader::CanLoad(const std::string& virtualPath) const
{
	std::vector<std::string> supportedExtensions = GetSupportedExtensions();

	for (const auto& ext : supportedExtensions)
	{
		if (virtualPath.size() >= ext.size() && virtualPath.compare(virtualPath.size() - ext.size(), ext.size(), ext) == 0)
		{
			return true;
		}
	}
	return false;
}

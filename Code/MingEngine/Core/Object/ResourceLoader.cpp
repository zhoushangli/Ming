#include "MingEngine/Core/Object/ResourceLoader.hpp"

int                                            ResourceLoader::s_loaderCount = 0;
Ref<ResourceFormatLoader>                      ResourceLoader::s_loader[MaxLoaders];
std::unordered_map<std::string, Ref<Resource>> ResourceLoader::s_loadedResources;

void ResourceLoader::AddLoader(Ref<ResourceFormatLoader> loader) { s_loader[s_loaderCount++] = loader; }

Ref<Resource> ResourceLoader::Load(const std::string& path)
{
	Ref<Resource> resource;

	if (s_loadedResources.find(path) != s_loadedResources.end())
	{
		return s_loadedResources[path];
	}

	for (int i = 0; i < s_loaderCount; ++i)
	{
		if (s_loader[i]->CanLoad(path))
		{
			resource                = s_loader[i]->Load(path);
			s_loadedResources[path] = resource;
			return resource;
		}
	}

	return resource;
}

bool ResourceFormatLoader::CanLoad(const std::string& path) const
{
	std::vector<std::string> supportedExtensions = GetSupportedExtensions();

	for (const auto& ext : supportedExtensions)
	{
		if (path.size() >= ext.size() && path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
		{
			return true;
		}
	}
	return false;
}

std::vector<std::string> ScriptLoader::GetSupportedExtensions() const { return std::vector<std::string>({ ".as" }); }

Ref<Resource> ScriptLoader::Load(const std::string& virtualPath)
{
	Ref<Script> script = Ref<Script>(new Script());
	script->SetVirtualPath(virtualPath);

	size_t      slash = virtualPath.find_last_of("/\\");
	std::string name  = slash == std::string::npos ? virtualPath : virtualPath.substr(slash + 1);
	script->SetName(name);

	return script;
}

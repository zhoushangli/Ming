#include "MingEngine/Core/Object/ScriptLoader.hpp"

#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

std::vector<std::string> ScriptLoader::GetSupportedExtensions() const { return std::vector<std::string>({ ".as" }); }

Ref<Resource> ScriptLoader::Load(const std::string& virtualPath)
{
	std::string relativePath;
	if (!FileSystem::TryGetRelativePath(virtualPath, relativePath))
	{
		return Ref<Resource>();
	}

	Ref<Script> script = Ref<Script>(new Script());
	script->SetVirtualPath(virtualPath);

	size_t      slash = relativePath.find_last_of('/');
	std::string name  = slash == std::string::npos ? relativePath : relativePath.substr(slash + 1);

	size_t dot = name.find_last_of('.');
	if (dot != std::string::npos)
	{
		name = name.substr(0, dot);
	}

	script->SetName(name);

	return script;
}

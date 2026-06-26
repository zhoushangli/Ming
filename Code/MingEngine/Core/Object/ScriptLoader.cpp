#include "MingEngine/Core/Object/ScriptLoader.hpp"

#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Engine/File/VirtualPath.hpp"

std::vector<std::string> ScriptLoader::GetSupportedExtensions() const { return std::vector<std::string>({ ".as" }); }

Ref<Resource> ScriptLoader::Load(const std::string& virtualPath)
{
	VirtualPath parsedPath;
	if (!parsedPath.Parse(virtualPath))
	{
		return Ref<Resource>();
	}

	Ref<Script> script = Ref<Script>(new Script());
	script->SetVirtualPath(parsedPath.ToString());

	std::string const relativePath = parsedPath.GetRelativePath();
	size_t            slash        = relativePath.find_last_of('/');
	std::string       name         = slash == std::string::npos ? relativePath : relativePath.substr(slash + 1);
	script->SetName(name);

	return script;
}
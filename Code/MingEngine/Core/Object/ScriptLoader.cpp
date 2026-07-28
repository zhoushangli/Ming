#include "MingEngine/Core/Object/ScriptLoader.hpp"

#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

std::vector<std::string> ScriptLoader::GetSupportedExtensions() const { return std::vector<std::string>({ ".as" }); }

Ref<Resource> ScriptLoader::Load(VirtualPath const& virtualPath)
{
	if (!virtualPath.IsValid() || virtualPath.IsRoot())
	{
		return Ref<Resource>();
	}

	Ref<Script> script = Ref<Script>(new Script());
	script->SetVirtualPath(virtualPath);

	script->SetName(virtualPath.GetStem());

	return script;
}

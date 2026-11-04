#include "MingEngine/Engine/Script/CSharpScriptFormat.hpp"
#include "MingEngine/Engine/Script/CSharpScript.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

std::vector<std::string> CSharpScriptLoader::GetSupportedExtensions() const
{
	return std::vector<std::string>({ ".cs" });
}

Ref<Resource> CSharpScriptLoader::Load(VirtualPath const& virtualPath)
{
	if (g_engine == nullptr || !g_engine->m_fileSystem->Exists(virtualPath))
	{
		return {};
	}

	Ref<CSharpScript> script = CreateRef<CSharpScript>();
	script->SetVirtualPath(virtualPath);
	return script;
}

bool CSharpScriptSaver::CanSave(VirtualPath const& virtualPath, Variant const& value) const
{
	Ref<CSharpScript> csharpScript(value);
	return csharpScript.IsValid() && virtualPath.HasExtension(".cs");
}

bool CSharpScriptSaver::Save(VirtualPath const& virtualPath, Variant const& value)
{
	return true;
}

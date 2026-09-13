#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

class CSharpScriptLoader : public ResourceFormatLoader
{
	MCLASS(CSharpScriptLoader, ResourceFormatLoader)

public:
	virtual std::vector<std::string> GetSupportedExtensions() const override;
	virtual Ref<Resource>            Load(VirtualPath const& virtualPath) override;

protected:
	static void BindMethods() {};
};

class CSharpScriptSaver : public ResourceFormatSaver
{
	MCLASS(CSharpScriptSaver, ResourceFormatSaver)

public:
	virtual bool CanSave(VirtualPath const& virtualPath, Variant const& value) const override;
	virtual bool Save(VirtualPath const& virtualPath, Variant const& value) override;

protected:
	static void BindMethods() {};
};
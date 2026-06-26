#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"

class ScriptLoader : public ResourceFormatLoader
{
	MCLASS(ScriptLoader, ResourceFormatLoader)

public:
	virtual std::vector<std::string> GetSupportedExtensions() const override;
	virtual Ref<Resource>            Load(const std::string& virtualPath) override;

protected:
	static void BindMethods() {};
};
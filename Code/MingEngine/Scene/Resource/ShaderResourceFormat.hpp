#pragma once

#include "MingEngine/Core/IO/ResourceLoader.hpp"

class ShaderResourceLoader : public ResourceFormatLoader
{
	MCLASS(ShaderResourceLoader, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource>            Load(VirtualPath const& virtualPath) override;

protected:
	static void BindMethods() {};
};

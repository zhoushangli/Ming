#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"

class ShaderResourceLoader : public ResourceFormatLoader
{
	MCLASS(ShaderResourceLoader, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource>            Load(std::string const& virtualPath) override;

protected:
	static void BindMethods() {};
};
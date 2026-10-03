#pragma once

#include "MingEngine/Core/Object/ResourceLoader.hpp"

class FontResourceLoader : public ResourceFormatLoader
{
	MCLASS(FontResourceLoader, ResourceFormatLoader)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	Ref<Resource>            Load(VirtualPath const& path) override;

protected:
	static void BindMethods() {}
};
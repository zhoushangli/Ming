#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"

class GLTFImporter : public ResourceFormatImporter
{
	MCLASS(GLTFImporter, ResourceFormatImporter)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	std::string              GetVisibleName() const override;
	std::string              GetImportedExtension() const override;

protected:
	Ref<Resource> Import(
		std::unordered_map<std::string, Variant> const& importOptions,
		std::string const&                              sourceVirtualPath) override;

protected:
	static void BindMethods() {};
};

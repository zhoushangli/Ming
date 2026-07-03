#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"

class GLTFImporter : public ResourceFormatImporter
{
	MCLASS(GLTFImporter, ResourceFormatImporter)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	std::string              GetImportedExtension() const override;
	Ref<Resource>            Import(std::string const& sourceVirtualPath) override;

protected:
	static void BindMethods() {};
};

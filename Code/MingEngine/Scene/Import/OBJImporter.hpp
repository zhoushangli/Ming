#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include <string>

class OBJImporter : public ResourceFormatImporter
{
	MCLASS(OBJImporter, ResourceFormatImporter)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	std::string              GetImportedExtension() const override;
	Ref<Resource>            Import(std::string const& sourceVirtualPath) override;

protected:
	static void BindMethods() {};
	
};

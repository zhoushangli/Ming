#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include <string>

class OBJImporter : public ResourceFormatImporter
{
	MCLASS(OBJImporter, ResourceFormatImporter)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	std::string              GetVisibleName() const override;
	std::string              GetImportedExtension() const override;
	std::vector<ImportOptions> const GetImportOptions() const override;

protected:
	Ref<Resource> Import(
		std::unordered_map<std::string, Variant> const& importOptions,
		VirtualPath const&                              sourceVirtualPath) override;

protected:
	static void BindMethods() {};
	
};

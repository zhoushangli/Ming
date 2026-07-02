#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include <string>

class OBJImporter : public ResourceFormatImporter
{
	MCLASS(OBJImporter, ResourceFormatImporter)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	bool Import(std::string const& sourceVirtualPath, std::string const& importVirtualPath) override;

protected:
	static void BindMethods() {};
	
};
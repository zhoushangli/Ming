#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"

class GLTFImporter : public ResourceFormatImporter
{
	MCLASS(GLTFImporter, ResourceFormatImporter)

public:
	std::vector<std::string> GetSupportedExtensions() const override;
	bool Import(std::string const& sourceVirtualPath, std::string const& importVirtualPath) override;

protected:
	static void BindMethods() {};
};
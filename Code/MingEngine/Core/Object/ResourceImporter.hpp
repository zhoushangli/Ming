#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"

#include <string>
#include <vector>

class ResourceFormatImporter : public RefCounted
{
	MCLASS(ResourceFormatImporter, RefCounted)

public:
	virtual std::vector<std::string> GetSupportedExtensions() const = 0;
	virtual std::string              GetImportedExtension() const    = 0;
	virtual std::string              GetImporterName() const         = 0;
	virtual int                      GetImporterVersion() const      = 0;
	virtual bool Import(std::string const& sourceVirtualPath, std::string const& importVirtualPath) = 0;

	bool CanImport(std::string const& virtualPath) const;

protected:
	static void BindMethods() {};
};

class ResourceImporter
{
public:
	static void AddImporter(Ref<ResourceFormatImporter> importer);
	static bool CanImport(std::string const& sourceVirtualPath);
	static bool Import(std::string const& sourceVirtualPath);

	static bool IsImportMetadataPath(std::string const& virtualPath);
	static bool IsInternalResourcePath(std::string const& virtualPath);
	static bool TryReadImportFile(std::string const& sourceVirtualPath, std::string& outImportVirtualPath);

	static std::string GetImportMetadataPath(std::string const& sourceVirtualPath);
	static std::string GetImportOutputPath(
		std::string const& sourceVirtualPath,
		ResourceFormatImporter const& importer);

private:
	static constexpr int MaxImporters = 64;

	static Ref<ResourceFormatImporter> FindImporter(std::string const& sourceVirtualPath);

private:
	static int                         s_importerCount;
	static Ref<ResourceFormatImporter> s_importer[MaxImporters];
};

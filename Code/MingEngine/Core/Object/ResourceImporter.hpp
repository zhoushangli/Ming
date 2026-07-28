#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct ImportOptions
{
	PropertyInfo m_propertyInfo;
	Variant      m_defaultValue;
};

class ResourceFormatImporter : public RefCounted
{
	MCLASS(ResourceFormatImporter, RefCounted)
	friend class ResourceImporter;

public:
	virtual std::vector<std::string>         GetSupportedExtensions() const = 0;
	virtual std::string                      GetVisibleName() const         = 0;
	virtual std::string                      GetImportedExtension() const;
	virtual std::vector<ImportOptions> const GetImportOptions() const;

	bool CanImport(VirtualPath const& virtualPath) const;

protected:
	// ResourceFormatImporter::Import only converts a source asset, such as an .obj file, into a Resource.
	// ResourceImporter::Import owns the full import pipeline: importer selection, output path, saving,
	// and .import metadata. Loaders/savers work with final resource formats, while importers work with
	// source assets and may need options because more than one importer can match a file.
	virtual Ref<Resource>
	Import(std::unordered_map<std::string, Variant> const& importOptions, VirtualPath const& sourceVirtualPath) = 0;

protected:
	static void BindMethods() {};
};

class ResourceImporter
{
public:
	static void                                     AddImporter(Ref<ResourceFormatImporter> importer);
	static std::vector<Ref<ResourceFormatImporter>> GetMatchedImporters(VirtualPath const& sourceVirtualPath);
	static bool                                     CanImport(VirtualPath const& sourceVirtualPath);
	static bool                                     Import(
		VirtualPath const&                              sourceVirtualPath,
		Ref<ResourceFormatImporter>                     importer,
		std::unordered_map<std::string, Variant> const& importOptions);
	static bool Import(VirtualPath const& sourceVirtualPath);

	static bool IsImportConfigPath(VirtualPath const& virtualPath);
	static bool IsInternalResourcePath(VirtualPath const& virtualPath);
	static bool TryGetImportFile(VirtualPath const& sourceVirtualPath, VirtualPath& outImportVirtualPath);
	static bool TryReadImportConfig(
		VirtualPath const&                        sourceVirtualPath,
		std::string&                              outImporterClassName,
		std::unordered_map<std::string, Variant>& outImportOptions);
	static bool EnsureImported(VirtualPath const& sourceVirtualPath);

	static VirtualPath GetImportConfigPath(VirtualPath const& sourceVirtualPath);
	static VirtualPath GetImportOutputPath(VirtualPath const& sourceVirtualPath, std::string const& importedExtension);

private:
	static constexpr int MaxImporters = 64;

	static std::vector<Ref<ResourceFormatImporter>> FindMatchedImporters(VirtualPath const& sourceVirtualPath);

private:
	static int                         s_importerCount;
	static Ref<ResourceFormatImporter> s_importer[MaxImporters];
};

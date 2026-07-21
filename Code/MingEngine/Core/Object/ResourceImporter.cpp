#include "MingEngine/Core/Object/ResourceImporter.hpp"

#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/Object/VariantJson.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#define XXH_INLINE_ALL
#include "ThirdParty/xxhash/xxhash.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace
{
using Json = nlohmann::ordered_json;

constexpr char const* kInternalResourcePathPrefix = "res://.ming/";
constexpr char const* kInternalImportDirectory    = "res://.ming/Import/";
constexpr char const* kImportConfigExtension      = ".import";
constexpr char const* kDefaultImportedExtension   = "mingres";

// For example, "res://foo/bar/baz.txt" -> "baz"
std::string GetImportStem(VirtualPath const& sourceVirtualPath)
{
	std::string stem = sourceVirtualPath.GetStem();
	if (stem.empty())
	{
		stem = "resource";
	}

	return stem;
}

std::string GetHashSuffix(VirtualPath const& sourceVirtualPath)
{
	std::string const& path = sourceVirtualPath.GetString();
	XXH64_hash_t const hash = XXH3_64bits(path.data(), path.size());

	std::ostringstream stream;
	stream << std::hex << std::setfill('0') << std::setw(16) << hash;
	return stream.str();
}

std::string NormalizeExtension(std::string extension)
{
	if (extension.empty())
	{
		extension = kDefaultImportedExtension;
	}

	if (!extension.empty() && extension.front() == '.')
	{
		extension.erase(extension.begin());
	}

	if (extension.empty())
	{
		extension = kDefaultImportedExtension;
	}

	return extension;
}

// This function reads and validates the JSON object stored beside a source asset.
// e.g. res://Models/Pawn.obj reads res://Models/Pawn.obj.import.
bool TryReadImportConfigJson(VirtualPath const& sourceVirtualPath, Json& outMetadata)
{
	outMetadata = Json::object();

	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	std::string text;
	if (!g_engine->m_fileSystem->ReadText(ResourceImporter::GetImportConfigPath(sourceVirtualPath), text))
	{
		return false;
	}

	Json metadata = Json::parse(text, nullptr, false);
	if (metadata.is_discarded() || !metadata.is_object())
	{
		return false;
	}

	if (!metadata.contains("source_file") || !metadata["source_file"].is_string()
		|| metadata["source_file"].get<std::string>() != sourceVirtualPath.GetString())
	{
		return false;
	}

	outMetadata = std::move(metadata);
	return true;
}

Ref<ResourceFormatImporter>
FindImporterByClassName(VirtualPath const& sourceVirtualPath, std::string const& importerClassName)
{
	for (Ref<ResourceFormatImporter> const& importer : ResourceImporter::GetMatchedImporters(sourceVirtualPath))
	{
		if (importer.IsValid() && importer->GetClassName() == importerClassName)
		{
			return importer;
		}
	}

	return Ref<ResourceFormatImporter>();
}

ImportOptions const* FindImportOption(std::vector<ImportOptions> const& options, std::string const& name)
{
	for (ImportOptions const& option : options)
	{
		if (option.m_propertyInfo.m_name == name)
		{
			return &option;
		}
	}

	return nullptr;
}

// This function serializes only user-changed import options.
// e.g. default scale is omitted, while a custom scale is written into import_options.
Json SerializeNonDefaultImportOptions(
	Ref<ResourceFormatImporter>                     importer,
	std::unordered_map<std::string, Variant> const& importOptions)
{
	Json serializedOptions = Json::object();
	if (!importer.IsValid())
	{
		return serializedOptions;
	}

	std::vector<ImportOptions> const defaultOptions = importer->GetImportOptions();
	for (ImportOptions const& defaultOption : defaultOptions)
	{
		std::string const& optionName = defaultOption.m_propertyInfo.m_name;
		auto const         iter       = importOptions.find(optionName);
		if (iter == importOptions.end())
		{
			continue;
		}

		Variant const& value = iter->second;
		if (value.GetType() != defaultOption.m_propertyInfo.m_type || value == defaultOption.m_defaultValue)
		{
			continue;
		}

		Json optionJson;
		if (VariantJson::TrySerialize(value, optionJson))
		{
			serializedOptions[optionName] = std::move(optionJson);
		}
	}

	return serializedOptions;
}

bool TryGetVirtualPathModifiedTime(
	VirtualPath const&               virtualPath,
	std::filesystem::file_time_type& outModifiedTime)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	std::filesystem::path physicalPath;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(virtualPath, physicalPath))
	{
		return false;
	}

	std::error_code errorCode;
	outModifiedTime = std::filesystem::last_write_time(physicalPath, errorCode);
	return !errorCode;
}

} // namespace

int                         ResourceImporter::s_importerCount = 0;
Ref<ResourceFormatImporter> ResourceImporter::s_importer[MaxImporters];

void ResourceImporter::AddImporter(Ref<ResourceFormatImporter> importer)
{
	if (importer.IsValid() && s_importerCount < MaxImporters)
	{
		s_importer[s_importerCount++] = importer;
	}
}

bool ResourceImporter::CanImport(VirtualPath const& sourceVirtualPath)
{
	return !FindMatchedImporters(sourceVirtualPath).empty();
}

std::vector<Ref<ResourceFormatImporter>> ResourceImporter::GetMatchedImporters(VirtualPath const& sourceVirtualPath)
{
	return FindMatchedImporters(sourceVirtualPath);
}

// This function runs the full import pipeline for a source asset.
// e.g. it converts res://Models/Pawn.obj, saves a generated mesh, and writes Pawn.obj.import.
bool ResourceImporter::Import(
	VirtualPath const&                              sourceVirtualPath,
	Ref<ResourceFormatImporter>                     importer,
	std::unordered_map<std::string, Variant> const& importOptions)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	if (!importer.IsValid() || !importer->CanImport(sourceVirtualPath))
	{
		return false;
	}

	Ref<Resource> importedResource = importer->Import(importOptions, sourceVirtualPath);
	if (!importedResource.IsValid())
	{
		return false;
	}

	VirtualPath const importPath = GetImportOutputPath(sourceVirtualPath, importer->GetImportedExtension());
	if (!ResourceSaver::Save(importPath, importedResource))
	{
		return false;
	}

	Json metadata;
	metadata["source_file"] = sourceVirtualPath.GetString();
	metadata["import_file"] = importPath.GetString();
	metadata["importer"]    = importer->GetClassName();

	Json serializedOptions = SerializeNonDefaultImportOptions(importer, importOptions);
	if (!serializedOptions.empty())
	{
		metadata["import_options"] = std::move(serializedOptions);
	}

	if (!g_engine->m_fileSystem->WriteText(GetImportConfigPath(sourceVirtualPath), metadata.dump(1, '\t')))
	{
		return false;
	}

	return true;
}

bool ResourceImporter::Import(VirtualPath const& sourceVirtualPath)
{
	std::vector<Ref<ResourceFormatImporter>> importers = FindMatchedImporters(sourceVirtualPath);
	if (importers.empty())
	{
		return false;
	}

	return Import(sourceVirtualPath, importers.front(), {});
}

bool ResourceImporter::IsImportConfigPath(VirtualPath const& virtualPath)
{
	return virtualPath.HasExtension(kImportConfigExtension);
}

bool ResourceImporter::IsInternalResourcePath(VirtualPath const& virtualPath)
{
	std::string const& path = virtualPath.GetString();
	return path == "res://.ming"
		   || path.compare(0, std::strlen(kInternalResourcePathPrefix), kInternalResourcePathPrefix) == 0;
}

// This function reads the imported cache path from the import config.
// e.g. res://Models/Pawn.obj.import can point to res://.ming/Import/Pawn_1234.mesh.
bool ResourceImporter::TryGetImportFile(VirtualPath const& sourceVirtualPath, VirtualPath& outImportVirtualPath)
{
	outImportVirtualPath = {};

	Json metadata;
	if (!TryReadImportConfigJson(sourceVirtualPath, metadata))
	{
		return false;
	}

	if (!metadata.contains("import_file") || !metadata["import_file"].is_string())
	{
		return false;
	}

	VirtualPath importPath;
	if (!VirtualPath::TryParse(metadata["import_file"].get<std::string>(), importPath))
	{
		return false;
	}

	outImportVirtualPath = std::move(importPath);
	return true;
}

// This function reads the import config for a source asset.
// e.g. res://Models/Pawn.obj reads importer/options from res://Models/Pawn.obj.import.
bool ResourceImporter::TryReadImportConfig(
	VirtualPath const&                        sourceVirtualPath,
	std::string&                              outImporterClassName,
	std::unordered_map<std::string, Variant>& outImportOptions)
{
	outImporterClassName.clear();
	outImportOptions.clear();

	Json metadata;
	if (!TryReadImportConfigJson(sourceVirtualPath, metadata))
	{
		return false;
	}

	if (!metadata.contains("importer") || !metadata["importer"].is_string())
	{
		return false;
	}

	outImporterClassName = metadata["importer"].get<std::string>();

	if (!metadata.contains("import_options"))
	{
		return true;
	}

	Json const& savedOptions = metadata["import_options"];
	if (!savedOptions.is_object())
	{
		return false;
	}

	Ref<ResourceFormatImporter> importer = FindImporterByClassName(sourceVirtualPath, outImporterClassName);
	if (!importer.IsValid())
	{
		return true;
	}

	std::vector<ImportOptions> const defaultOptions = importer->GetImportOptions();
	for (auto optionEntry = savedOptions.begin(); optionEntry != savedOptions.end(); ++optionEntry)
	{
		ImportOptions const* option = FindImportOption(defaultOptions, optionEntry.key());
		if (option == nullptr)
		{
			continue;
		}

		Variant value;
		if (VariantJson::TryDeserialize(optionEntry.value(), option->m_propertyInfo.m_type, value))
		{
			outImportOptions[optionEntry.key()] = std::move(value);
		}
	}

	return true;
}

// This function keeps the import config and generated cache complete for a source asset.
// e.g. a missing res://.ming/Import/Pawn_1234.mesh is regenerated from res://Models/Pawn.obj.
bool ResourceImporter::EnsureImported(VirtualPath const& sourceVirtualPath)
{
	std::vector<Ref<ResourceFormatImporter>> importers = FindMatchedImporters(sourceVirtualPath);
	if (importers.empty())
	{
		return false;
	}

	// 1) Read the source modified time so stale caches can be detected.
	std::filesystem::file_time_type sourceModifiedTime;
	if (!TryGetVirtualPathModifiedTime(sourceVirtualPath, sourceModifiedTime))
	{
		return false;
	}

	// 2) Read the import config and restore the saved importer/options when possible.
	std::string                              importerClassName;
	std::unordered_map<std::string, Variant> importOptions;
	bool const hasConfig = TryReadImportConfig(sourceVirtualPath, importerClassName, importOptions);

	Ref<ResourceFormatImporter> importer;
	if (hasConfig)
	{
		importer = FindImporterByClassName(sourceVirtualPath, importerClassName);
	}
	if (!importer.IsValid())
	{
		importer       = importers.front();
		importOptions  = {};
		importerClassName.clear();
	}

	// 3) Validate that the generated cache exists and is at least as new as the source.
	VirtualPath importPath;
	bool        hasFreshCache = false;
	if (hasConfig && TryGetImportFile(sourceVirtualPath, importPath) && g_engine != nullptr
		&& g_engine->m_fileSystem != nullptr && g_engine->m_fileSystem->Exists(importPath))
	{
		std::filesystem::file_time_type importModifiedTime;
		hasFreshCache =
			TryGetVirtualPathModifiedTime(importPath, importModifiedTime) && importModifiedTime >= sourceModifiedTime;
	}

	// 4) Reimport when the config is missing/broken, the importer is unavailable, or the cache is stale.
	if (hasConfig && importerClassName == importer->GetClassName() && hasFreshCache)
	{
		return true;
	}

	return Import(sourceVirtualPath, importer, importOptions);
}

// This function returns the import config path beside the source asset.
// e.g. res://Models/Pawn.obj becomes res://Models/Pawn.obj.import.
VirtualPath ResourceImporter::GetImportConfigPath(VirtualPath const& sourceVirtualPath)
{
	return VirtualPath(sourceVirtualPath.GetString() + kImportConfigExtension);
}

// This function returns the generated cache path under the internal import directory.
// e.g. res://Models/Pawn.obj can become res://.ming/Import/Pawn_1234.mesh.
VirtualPath
ResourceImporter::GetImportOutputPath(VirtualPath const& sourceVirtualPath, std::string const& importedExtension)
{
	return VirtualPath(std::string(kInternalImportDirectory) + GetImportStem(sourceVirtualPath) + "_"
		   + GetHashSuffix(sourceVirtualPath) + "." + NormalizeExtension(importedExtension));
}

std::vector<Ref<ResourceFormatImporter>> ResourceImporter::FindMatchedImporters(VirtualPath const& sourceVirtualPath)
{
	std::vector<Ref<ResourceFormatImporter>> matchedImporters;

	if (!sourceVirtualPath.IsValid() || IsInternalResourcePath(sourceVirtualPath)
		|| IsImportConfigPath(sourceVirtualPath))
	{
		return matchedImporters;
	}

	for (int i = 0; i < s_importerCount; ++i)
	{
		if (s_importer[i]->CanImport(sourceVirtualPath))
		{
			matchedImporters.push_back(s_importer[i]);
		}
	}

	return matchedImporters;
}

bool ResourceFormatImporter::CanImport(VirtualPath const& virtualPath) const
{
	for (std::string extension : GetSupportedExtensions())
	{
		if (virtualPath.HasExtension(extension))
		{
			return true;
		}
	}

	return false;
}

std::string ResourceFormatImporter::GetImportedExtension() const { return kDefaultImportedExtension; }

std::vector<ImportOptions> const ResourceFormatImporter::GetImportOptions() const { return {}; }

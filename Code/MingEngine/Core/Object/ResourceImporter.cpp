#include "MingEngine/Core/Object/ResourceImporter.hpp"

#include "MingEngine/Core/Object/ResourceSaver.hpp"
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
constexpr char const* kInternalResourcePathPrefix = "res://.ming/";
constexpr char const* kInternalImportDirectory    = "res://.ming/Import/";
constexpr char const* kImportMetadataExtension    = ".import";
constexpr char const* kDefaultImportedExtension   = "mingres";

std::string ToLower(std::string text)
{
	std::transform(
		text.begin(),
		text.end(),
		text.begin(),
		[](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return text;
}

bool EndsWith(std::string const& text, std::string const& suffix)
{
	return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// For example, "res://foo/bar/baz.txt" -> "baz"
std::string GetImportStem(std::string const& sourceVirtualPath)
{
	std::string relativePath;
	if (!FileSystem::TryGetRelativePath(sourceVirtualPath, relativePath))
	{
		return "resource";
	}

	std::string stem = std::filesystem::path(relativePath).stem().string();
	if (stem.empty())
	{
		stem = "resource";
	}

	return stem;
}

std::string GetHashSuffix(std::string const& sourceVirtualPath)
{
	XXH64_hash_t const hash = XXH3_64bits(sourceVirtualPath.data(), sourceVirtualPath.size());

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

} // namespace

int ResourceImporter::s_importerCount = 0;
Ref<ResourceFormatImporter> ResourceImporter::s_importer[MaxImporters];

void ResourceImporter::AddImporter(Ref<ResourceFormatImporter> importer)
{
	if (importer.IsValid() && s_importerCount < MaxImporters)
	{
		s_importer[s_importerCount++] = importer;
	}
}

bool ResourceImporter::CanImport(std::string const& sourceVirtualPath)
{
	return !FindMatchedImporters(sourceVirtualPath).empty();
}

std::vector<Ref<ResourceFormatImporter>> ResourceImporter::GetMatchedImporters(std::string const& sourceVirtualPath)
{
	return FindMatchedImporters(sourceVirtualPath);
}

bool ResourceImporter::Import(
	std::string const&                              sourceVirtualPath,
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

	std::string const importPath = GetImportOutputPath(sourceVirtualPath, importer->GetImportedExtension());
	if (!ResourceSaver::Save(importPath, importedResource))
	{
		return false;
	}

	nlohmann::ordered_json metadata;
	metadata["source_file"] = sourceVirtualPath;
	metadata["import_file"] = importPath;
	metadata["importer"]    = importer->GetClassName();

	if (!g_engine->m_fileSystem->WriteText(GetImportMetadataPath(sourceVirtualPath), metadata.dump(1, '\t')))
	{
		return false;
	}

	return true;
}

bool ResourceImporter::Import(std::string const& sourceVirtualPath)
{
	std::vector<Ref<ResourceFormatImporter>> importers = FindMatchedImporters(sourceVirtualPath);
	if (importers.empty())
	{
		return false;
	}

	return Import(sourceVirtualPath, importers.front(), {});
}

bool ResourceImporter::IsImportMetadataPath(std::string const& virtualPath)
{
	return EndsWith(ToLower(virtualPath), kImportMetadataExtension);
}

bool ResourceImporter::IsInternalResourcePath(std::string const& virtualPath)
{
	return virtualPath == "res://.ming" || virtualPath.compare(0, std::strlen(kInternalResourcePathPrefix), kInternalResourcePathPrefix) == 0;
}

bool ResourceImporter::TryReadImportFile(std::string const& sourceVirtualPath, std::string& outImportVirtualPath)
{
	outImportVirtualPath.clear();

	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	std::string text;
	if (!g_engine->m_fileSystem->ReadText(GetImportMetadataPath(sourceVirtualPath), text))
	{
		return false;
	}

	nlohmann::ordered_json metadata = nlohmann::ordered_json::parse(text, nullptr, false);
	if (metadata.is_discarded() || !metadata.is_object())
	{
		return false;
	}

	if (!metadata.contains("source_file") || !metadata.contains("import_file"))
	{
		return false;
	}

	if (!metadata["source_file"].is_string() || !metadata["import_file"].is_string())
	{
		return false;
	}

	if (metadata["source_file"].get<std::string>() != sourceVirtualPath)
	{
		return false;
	}

	std::string importPath = metadata["import_file"].get<std::string>();
	if (!FileSystem::IsVirtualPath(importPath))
	{
		return false;
	}

	outImportVirtualPath = std::move(importPath);
	return true;
}

std::string ResourceImporter::GetImportMetadataPath(std::string const& sourceVirtualPath)
{
	return sourceVirtualPath + kImportMetadataExtension;
}

std::string ResourceImporter::GetImportOutputPath(
	std::string const& sourceVirtualPath,
	std::string const& importedExtension)
{
	return std::string(kInternalImportDirectory)
		+ GetImportStem(sourceVirtualPath)
		+ "_"
		+ GetHashSuffix(sourceVirtualPath)
		+ "."
		+ NormalizeExtension(importedExtension);
}

std::vector<Ref<ResourceFormatImporter>> ResourceImporter::FindMatchedImporters(std::string const& sourceVirtualPath)
{
	std::vector<Ref<ResourceFormatImporter>> matchedImporters;

	if (!FileSystem::IsVirtualPath(sourceVirtualPath)
		|| IsInternalResourcePath(sourceVirtualPath)
		|| IsImportMetadataPath(sourceVirtualPath))
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

bool ResourceFormatImporter::CanImport(std::string const& virtualPath) const
{
	std::string const lowerPath = ToLower(virtualPath);

	for (std::string extension : GetSupportedExtensions())
	{
		extension = ToLower(std::move(extension));
		if (!extension.empty() && extension.front() != '.')
		{
			extension.insert(extension.begin(), '.');
		}

		if (!extension.empty() && EndsWith(lowerPath, extension))
		{
			return true;
		}
	}

	return false;
}

std::string ResourceFormatImporter::GetImportedExtension() const { return kDefaultImportedExtension; }

std::vector<ImportOptions> const ResourceFormatImporter::GetImportOptions() const { return {}; }

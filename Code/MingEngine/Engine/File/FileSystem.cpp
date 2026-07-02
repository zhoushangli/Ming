#include "MingEngine/Engine/File/FileSystem.hpp"

#include "MingEngine/Core/Object/ResourceImporter.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{
constexpr char const* kResourcePathPrefix = "res://";
constexpr char const* kInternalResourceDirectoryName = ".ming";
constexpr char const* kImportMetadataExtension = ".import";

std::string ToLower(std::string text)
{
	std::transform(
		text.begin(),
		text.end(),
		text.begin(),
		[](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return text;
}

std::string GetDisplayName(std::filesystem::path const& path)
{
	std::string displayName = path.filename().string();
	if (displayName.empty())
	{
		displayName = path.string();
	}
	return displayName;
}

std::string JoinVirtualPath(std::string const& parentVirtualPath, std::string const& name)
{
	if (parentVirtualPath == kResourcePathPrefix)
	{
		return parentVirtualPath + name;
	}

	return parentVirtualPath + "/" + name;
}

bool IsImportMetadataFile(std::filesystem::path const& path)
{
	return path.extension().string() == kImportMetadataExtension;
}

bool IsInternalResourceDirectory(std::filesystem::path const& path)
{
	return path.filename().string() == kInternalResourceDirectoryName;
}

bool ShouldSkipResourceTreeEntry(std::filesystem::path const& path, bool isDirectory)
{
	return (isDirectory && IsInternalResourceDirectory(path)) || (!isDirectory && IsImportMetadataFile(path));
}

bool TryGetLastWriteTime(std::filesystem::path const& path, std::filesystem::file_time_type& outLastWriteTime)
{
	std::error_code errorCode;
	outLastWriteTime = std::filesystem::last_write_time(path, errorCode);
	return !errorCode;
}
} // namespace

FileEntry::FileEntry(
	std::filesystem::path physicalPath,
	std::string           virtualPath,
	std::string           name,
	std::string           lowerName,
	bool                  isDirectory,
	FileEntry*            parent)
	: m_physicalPath(std::move(physicalPath)), m_virtualPath(std::move(virtualPath)), m_name(std::move(name)),
	  m_lowerName(std::move(lowerName)), m_isDirectory(isDirectory), m_parent(parent)
{
}

std::filesystem::path const&                   FileEntry::GetPhysicalPath() const { return m_physicalPath; }
std::string const&                             FileEntry::GetVirtualPath() const { return m_virtualPath; }
std::string const&                             FileEntry::GetName() const { return m_name; }
std::string const&                             FileEntry::GetLowerName() const { return m_lowerName; }
bool                                           FileEntry::IsDirectory() const { return m_isDirectory; }
FileEntry const*                               FileEntry::GetParent() const { return m_parent; }
std::vector<std::unique_ptr<FileEntry>> const& FileEntry::GetChildren() const { return m_children; }
bool                                           FileEntry::HasModifiedTime() const { return m_hasModifiedTime; }
bool                                           FileEntry::HasImportTime() const { return m_hasImportTime; }
std::filesystem::file_time_type                FileEntry::GetModifiedTime() const { return m_modifiedTime; }
std::filesystem::file_time_type                FileEntry::GetImportTime() const { return m_importTime; }

namespace
{
FileEntry const* FindEntryInTree(FileEntry const* rootEntry, std::string const& virtualPath)
{
	if (rootEntry == nullptr)
	{
		return nullptr;
	}

	if (rootEntry->GetVirtualPath() == virtualPath)
	{
		return rootEntry;
	}

	for (std::unique_ptr<FileEntry> const& child : rootEntry->GetChildren())
	{
		FileEntry const* found = FindEntryInTree(child.get(), virtualPath);
		if (found != nullptr)
		{
			return found;
		}
	}

	return nullptr;
}
} // namespace

FileSystem::FileSystem(FileSystemConfig const& config) : m_resourceRoot(config.m_resourceRoot) {}

void FileSystem::BindMethods() {}

void FileSystem::Startup() { ScanResourceTree(); }
void FileSystem::Shutdown() {}
void FileSystem::BeginFrame() {}
void FileSystem::EndFrame() {}

bool FileSystem::IsVirtualPath(std::string const& path)
{
	std::string relativePath;
	return TryGetRelativePath(path, relativePath);
}

bool FileSystem::TryGetRelativePath(std::string const& virtualPath, std::string& outRelativePath)
{
	outRelativePath.clear();

	if (virtualPath.empty())
	{
		return false;
	}

	std::string const prefix = kResourcePathPrefix;

	if (virtualPath.compare(0, prefix.size(), prefix) != 0)
	{
		return false;
	}

	outRelativePath = virtualPath.substr(prefix.size());

	if (outRelativePath.empty())
	{
		return false;
	}

	if (outRelativePath.find('\\') != std::string::npos)
	{
		return false;
	}

	if (outRelativePath == ".." || outRelativePath.compare(0, 3, "../") == 0
		|| outRelativePath.find("/../") != std::string::npos
		|| (outRelativePath.size() >= 3 && outRelativePath.compare(outRelativePath.size() - 3, 3, "/..") == 0))
	{
		outRelativePath.clear();
		return false;
	}

	return true;
}

bool FileSystem::Exists(std::string const& virtualPath) const
{
	std::filesystem::path physicalPath;
	if (!ResolvePath(virtualPath, physicalPath))
	{
		return false;
	}

	return std::filesystem::exists(physicalPath) && std::filesystem::is_regular_file(physicalPath);
}

bool FileSystem::ReadText(std::string const& virtualPath, std::string& outText) const
{
	outText.clear();

	std::filesystem::path physicalPath;
	if (!ResolvePath(virtualPath, physicalPath))
	{
		return false;
	}

	std::ifstream file(physicalPath);
	if (!file.is_open())
	{
		return false;
	}

	std::ostringstream stream;
	stream << file.rdbuf();

	if (file.bad())
	{
		return false;
	}

	outText = stream.str();
	return true;
}
bool FileSystem::WriteText(std::string const& virtualPath, std::string const& text) const
{
	std::filesystem::path physicalPath;
	if (!ResolvePath(virtualPath, physicalPath))
	{
		return false;
	}

	std::error_code errorCode;
	std::filesystem::path const parentPath = physicalPath.parent_path();
	if (!parentPath.empty())
	{
		std::filesystem::create_directories(parentPath, errorCode);
		if (errorCode)
		{
			return false;
		}
	}

	std::ofstream file(physicalPath);
	if (!file.is_open())
	{
		return false;
	}

	file << text;
	return file.good();
}

std::filesystem::path const& FileSystem::GetResourceRoot() const { return m_resourceRoot; }

void FileSystem::ScanResourceTree()
{
	std::error_code errorCode;
	if (!std::filesystem::exists(m_resourceRoot, errorCode)
		|| !std::filesystem::is_directory(m_resourceRoot, errorCode))
	{
		m_rootEntry.reset();
		return;
	}

	// 1) Refresh import cache first, while the previous resource tree still holds import times.
	std::unordered_map<std::string, std::filesystem::file_time_type> importedTimes;
	ScanResourceImports(importedTimes);

	// 2) Move the previous tree aside so import times can be copied into the new tree.
	std::unique_ptr<FileEntry> previousRootEntry = std::move(m_rootEntry);

	// 3) Rebuild the visible resource tree, hiding generated import metadata/cache entries.
	m_rootEntry =
		std::unique_ptr<FileEntry>(
			new FileEntry(m_resourceRoot, kResourcePathPrefix, kResourcePathPrefix, kResourcePathPrefix, true, nullptr));
	m_rootEntry->m_hasModifiedTime = TryGetLastWriteTime(m_resourceRoot, m_rootEntry->m_modifiedTime);

	for (std::filesystem::directory_entry const& entry : std::filesystem::directory_iterator(m_resourceRoot, errorCode))
	{
		std::error_code entryError;
		bool const      isDirectory = entry.is_directory(entryError);
		bool const      isFile      = entry.is_regular_file(entryError);
		if (entryError || (!isDirectory && !isFile))
		{
			continue;
		}
		if (ShouldSkipResourceTreeEntry(entry.path(), isDirectory))
		{
			continue;
		}

		std::string const name = GetDisplayName(entry.path());
		std::string const childVirtualPath = JoinVirtualPath(kResourcePathPrefix, name);
		std::unique_ptr<FileEntry> childEntry =
			BuildEntry(
				entry.path(),
				childVirtualPath,
				m_rootEntry.get(),
				isDirectory,
				FindEntryInTree(previousRootEntry.get(), childVirtualPath),
				importedTimes);
		if (childEntry)
		{
			m_rootEntry->m_children.push_back(std::move(childEntry));
		}
	}

	SortChildren(*m_rootEntry);
}

bool FileSystem::HasResourceTree() const { return m_rootEntry != nullptr; }

FileEntry const* FileSystem::GetResourceRootEntry() const { return m_rootEntry.get(); }

std::string FileSystem::ToVirtualPath(std::filesystem::path const& physicalPath) const
{
	std::error_code       errorCode;
	std::filesystem::path relativePath = std::filesystem::relative(physicalPath, m_resourceRoot, errorCode);
	if (errorCode || relativePath.empty() || relativePath == ".")
	{
		return kResourcePathPrefix;
	}

	return std::string(kResourcePathPrefix) + relativePath.generic_string();
}

bool FileSystem::ResolvePath(std::string const& virtualPath, std::filesystem::path& outPhysicalPath) const
{
	outPhysicalPath.clear();

	std::string relativePath;
	if (!TryGetRelativePath(virtualPath, relativePath))
	{
		return false;
	}

	outPhysicalPath = m_resourceRoot / relativePath;

	return true;
}

FileEntry const* FileSystem::FindEntry(std::string const& virtualPath) const
{
	return FindEntryInTree(m_rootEntry.get(), virtualPath);
}

void FileSystem::ScanResourceImports(std::unordered_map<std::string, std::filesystem::file_time_type>& outImportedTimes)
{
	std::error_code errorCode;
	for (std::filesystem::recursive_directory_iterator it(m_resourceRoot, errorCode), end; it != end; it.increment(errorCode))
	{
		if (errorCode)
		{
			errorCode.clear();
			continue;
		}

		std::filesystem::directory_entry const& entry = *it;

		std::error_code entryError;
		bool const      isDirectory = entry.is_directory(entryError);
		bool const      isFile      = entry.is_regular_file(entryError);
		if (entryError || (!isDirectory && !isFile))
		{
			continue;
		}

		if (isDirectory && IsInternalResourceDirectory(entry.path()))
		{
			it.disable_recursion_pending();
			continue;
		}

		if (!isFile || IsImportMetadataFile(entry.path()))
		{
			continue;
		}

		std::string const virtualPath = ToVirtualPath(entry.path());
		if (!ResourceImporter::CanImport(virtualPath))
		{
			continue;
		}

		// 1) Read the current source modified time from the filesystem.
		std::filesystem::file_time_type modifiedTime;
		if (!TryGetLastWriteTime(entry.path(), modifiedTime))
		{
			continue;
		}

		FileEntry const* previousEntry = FindEntry(virtualPath);

		// 2) Compare metadata, imported cache existence, and the previous FileEntry import time.
		std::string importPath;
		bool const  hasMetadata    = ResourceImporter::TryReadImportFile(virtualPath, importPath);
		bool const  hasImportFile  = hasMetadata && Exists(importPath);
		bool const  hasImportTime  = previousEntry != nullptr && previousEntry->HasImportTime();
		bool const  isImportDirty  = !hasImportTime || previousEntry->GetImportTime() != modifiedTime;

		// 3) Import dirty resources and report the import time for the next rebuilt FileEntry.
		if (!hasMetadata || !hasImportFile || isImportDirty)
		{
			if (ResourceImporter::Import(virtualPath))
			{
				outImportedTimes[virtualPath] = modifiedTime;
			}
		}
		else
		{
			outImportedTimes[virtualPath] = previousEntry->GetImportTime();
		}
	}
}

std::unique_ptr<FileEntry> FileSystem::BuildEntry(
	std::filesystem::path const& physicalPath,
	std::string const&           virtualPath,
	FileEntry*                   parent,
	bool                         isDirectory,
	FileEntry const*             previousEntry,
	std::unordered_map<std::string, std::filesystem::file_time_type> const& importedTimes) const
{
	if (ShouldSkipResourceTreeEntry(physicalPath, isDirectory))
	{
		return nullptr;
	}

	std::string const          name = GetDisplayName(physicalPath);
	std::unique_ptr<FileEntry> result(
		new FileEntry(physicalPath, virtualPath, name, ToLower(name), isDirectory, parent));

	// 1) Store the current filesystem modified time on every visible entry.
	result->m_hasModifiedTime = TryGetLastWriteTime(physicalPath, result->m_modifiedTime);

	// 2) Store import time from this scan, or carry it over from the previous tree.
	auto importedTime = importedTimes.find(virtualPath);
	if (importedTime != importedTimes.end())
	{
		result->m_hasImportTime = true;
		result->m_importTime    = importedTime->second;
	}
	else if (previousEntry != nullptr && previousEntry->HasImportTime())
	{
		result->m_hasImportTime = true;
		result->m_importTime    = previousEntry->GetImportTime();
	}

	// 3) Recurse into visible children only; .ming and *.import stay addressable but hidden.
	if (isDirectory)
	{
		std::error_code errorCode;
		for (std::filesystem::directory_entry const& child :
			 std::filesystem::directory_iterator(physicalPath, errorCode))
		{
			std::error_code entryError;
			bool const      childIsDirectory = child.is_directory(entryError);
			bool const      childIsFile      = child.is_regular_file(entryError);
			if (entryError || (!childIsDirectory && !childIsFile))
			{
				continue;
			}
			if (ShouldSkipResourceTreeEntry(child.path(), childIsDirectory))
			{
				continue;
			}

			std::string const childName = GetDisplayName(child.path());
			std::string const childVirtualPath = JoinVirtualPath(virtualPath, childName);
			std::unique_ptr<FileEntry> childEntry =
				BuildEntry(
					child.path(),
					childVirtualPath,
					result.get(),
					childIsDirectory,
					FindEntryInTree(previousEntry, childVirtualPath),
					importedTimes);
			if (childEntry)
			{
				result->m_children.push_back(std::move(childEntry));
			}
		}

		SortChildren(*result);
	}

	return result;
}

void FileSystem::SortChildren(FileEntry& entry) const
{
	std::sort(
		entry.m_children.begin(),
		entry.m_children.end(),
		[](std::unique_ptr<FileEntry> const& a, std::unique_ptr<FileEntry> const& b)
		{
			if (a->IsDirectory() != b->IsDirectory())
			{
				return a->IsDirectory() && !b->IsDirectory();
			}

			return a->GetLowerName() < b->GetLowerName();
		});
}

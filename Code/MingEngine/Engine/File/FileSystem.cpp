#include "MingEngine/Engine/File/FileSystem.hpp"

#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace
{
constexpr char const* kInternalResourceDirectoryName = ".ming";
constexpr char const* kImportConfigExtension         = ".import";

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

bool IsImportMetadataFile(std::filesystem::path const& path)
{
	return path.extension().string() == kImportConfigExtension;
}

bool IsInternalResourceDirectory(std::filesystem::path const& path)
{
	return path.filename().string() == kInternalResourceDirectoryName;
}

bool ShouldSkipResourceTreeEntry(std::filesystem::path const& path, bool isDirectory)
{
	return (isDirectory && IsInternalResourceDirectory(path)) || (!isDirectory && IsImportMetadataFile(path));
}

bool IsVisibleResourceFile(VirtualPath const& virtualPath)
{
	return ResourceLoader::CanLoad(virtualPath) || ResourceImporter::CanImport(virtualPath);
}

bool TryGetLastWriteTime(std::filesystem::path const& path, std::filesystem::file_time_type& outLastWriteTime)
{
	std::error_code errorCode;
	outLastWriteTime = std::filesystem::last_write_time(path, errorCode);
	return !errorCode;
}

bool IsValidEntryName(std::string const& name)
{
	if (name.empty() || name == "." || name == ".." || name.front() == ' ' || name.back() == ' ' || name.back() == '.')
	{
		return false;
	}

	if (name.find_first_of("<>:\"/\\|?*") != std::string::npos)
	{
		return false;
	}

	std::string const upperStem = ToLower(std::filesystem::path(name).stem().string());
	if (upperStem == "con" || upperStem == "prn" || upperStem == "aux" || upperStem == "nul")
	{
		return false;
	}
	if (upperStem.size() == 4 && (upperStem.compare(0, 3, "com") == 0 || upperStem.compare(0, 3, "lpt") == 0)
		&& upperStem[3] >= '1' && upperStem[3] <= '9')
	{
		return false;
	}
	return true;
}

std::string JoinError(std::string const& operation, std::error_code const& errorCode)
{
	return operation + ": " + errorCode.message();
}
} // namespace

FileEntry::FileEntry(
	std::filesystem::path physicalPath,
	VirtualPath           virtualPath,
	std::string           name,
	std::string           lowerName,
	bool                  isDirectory,
	FileEntry*            parent)
	: m_physicalPath(std::move(physicalPath)), m_virtualPath(std::move(virtualPath)), m_name(std::move(name)),
	  m_lowerName(std::move(lowerName)), m_isDirectory(isDirectory), m_parent(parent)
{
}

std::filesystem::path const&                   FileEntry::GetPhysicalPath() const { return m_physicalPath; }
VirtualPath const&                             FileEntry::GetVirtualPath() const { return m_virtualPath; }
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
FileEntry const* FindEntryInTree(FileEntry const* rootEntry, VirtualPath const& virtualPath)
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

bool FileSystem::Exists(VirtualPath const& virtualPath) const
{
	std::filesystem::path physicalPath;
	if (!TryGetPhysicalPath(virtualPath, physicalPath))
	{
		return false;
	}

	std::error_code errorCode;
	return std::filesystem::exists(physicalPath, errorCode) && !errorCode;
}

bool FileSystem::CreateFolder(
	VirtualPath const& parentVirtualPath,
	std::string const& name,
	VirtualPath&       outVirtualPath,
	std::string&       outError) const
{
	outVirtualPath = {};
	outError.clear();
	if (!IsValidEntryName(name))
	{
		outError = "Folder name is invalid.";
		return false;
	}

	std::filesystem::path parentPath;
	if (!TryGetWritablePhysicalPath(parentVirtualPath, parentPath, outError, true))
	{
		return false;
	}

	std::filesystem::path const targetPath = parentPath / name;
	std::error_code             errorCode;
	if (std::filesystem::exists(targetPath, errorCode))
	{
		outError = "An entry with this name already exists.";
		return false;
	}
	if (errorCode || !std::filesystem::create_directory(targetPath, errorCode))
	{
		outError = JoinError("Failed to create folder", errorCode);
		return false;
	}

	return TryToVirtualPath(targetPath, outVirtualPath);
}

bool FileSystem::Rename(
	VirtualPath const& virtualPath,
	std::string const& newName,
	VirtualPath&       outVirtualPath,
	std::string&       outError) const
{
	outVirtualPath = {};
	outError.clear();
	if (!IsValidEntryName(newName))
	{
		outError = "Name is invalid.";
		return false;
	}

	std::filesystem::path sourcePath;
	if (!TryGetWritablePhysicalPath(virtualPath, sourcePath, outError))
	{
		return false;
	}

	std::filesystem::path const targetPath = sourcePath.parent_path() / newName;
	std::error_code             errorCode;
	if (std::filesystem::exists(targetPath, errorCode))
	{
		outError = "An entry with this name already exists.";
		return false;
	}
	std::filesystem::rename(sourcePath, targetPath, errorCode);
	if (errorCode)
	{
		outError = JoinError("Failed to rename entry", errorCode);
		return false;
	}

	return TryToVirtualPath(targetPath, outVirtualPath);
}

bool FileSystem::Duplicate(VirtualPath const& virtualPath, VirtualPath& outVirtualPath, std::string& outError) const
{
	outVirtualPath = {};
	outError.clear();
	std::filesystem::path sourcePath;
	if (!TryGetWritablePhysicalPath(virtualPath, sourcePath, outError))
	{
		return false;
	}

	std::filesystem::path const parentPath = sourcePath.parent_path();
	std::error_code errorCode;
	bool const isDirectory = std::filesystem::is_directory(sourcePath, errorCode);
	if (errorCode)
	{
		outError = JoinError("Failed to inspect entry", errorCode);
		return false;
	}
	std::string const stem = isDirectory ? sourcePath.filename().string() : sourcePath.stem().string();
	std::string const extension = isDirectory ? "" : sourcePath.extension().string();
	std::filesystem::path targetPath = parentPath / (stem + " Copy" + extension);
	for (uint32_t index = 2; std::filesystem::exists(targetPath, errorCode) && !errorCode; ++index)
	{
		targetPath = parentPath / (stem + " Copy " + std::to_string(index) + extension);
	}
	if (errorCode)
	{
		outError = JoinError("Failed to find a duplicate name", errorCode);
		return false;
	}

	std::filesystem::copy(sourcePath, targetPath, std::filesystem::copy_options::recursive, errorCode);
	if (errorCode)
	{
		outError = JoinError("Failed to duplicate entry", errorCode);
		return false;
	}

	return TryToVirtualPath(targetPath, outVirtualPath);
}

bool FileSystem::Remove(VirtualPath const& virtualPath, std::string& outError) const
{
	outError.clear();
	std::filesystem::path physicalPath;
	if (!TryGetWritablePhysicalPath(virtualPath, physicalPath, outError))
	{
		return false;
	}

	std::error_code errorCode;
	uintmax_t const removedCount = std::filesystem::remove_all(physicalPath, errorCode);
	if (errorCode || removedCount == 0)
	{
		outError = errorCode ? JoinError("Failed to delete entry", errorCode) : "Entry does not exist.";
		return false;
	}
	return true;
}

bool FileSystem::ReadText(VirtualPath const& virtualPath, std::string& outText) const
{
	outText.clear();

	std::filesystem::path physicalPath;
	if (!TryGetPhysicalPath(virtualPath, physicalPath))
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
bool FileSystem::WriteText(VirtualPath const& virtualPath, std::string const& text) const
{
	std::filesystem::path physicalPath;
	if (!TryGetPhysicalPath(virtualPath, physicalPath))
	{
		return false;
	}

	std::error_code             errorCode;
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

bool FileSystem::ReadBinary(VirtualPath const& virtualPath, std::vector<uint8_t>& outData) const
{
	outData.clear();

	std::filesystem::path physicalPath;
	if (!TryGetPhysicalPath(virtualPath, physicalPath))
	{
		return false;
	}

	std::ifstream file(physicalPath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		return false;
	}

	std::streamsize const size = file.tellg();
	if (size <= 0)
	{
		return false;
	}

	outData.resize(static_cast<size_t>(size));

	file.seekg(0, std::ios::beg);
	if (!outData.empty())
	{
		file.read(reinterpret_cast<char*>(outData.data()), size);
	}

	return file.good();
}

bool FileSystem::WriteBinary(VirtualPath const& virtualPath, std::vector<uint8_t> const& data) const
{
	std::filesystem::path physicalPath;
	if (!TryGetPhysicalPath(virtualPath, physicalPath))
	{
		return false;
	}

	std::error_code             errorCode;
	std::filesystem::path const parentPath = physicalPath.parent_path();
	if (!parentPath.empty())
	{
		std::filesystem::create_directories(parentPath, errorCode);
		if (errorCode)
		{
			return false;
		}
	}

	std::ofstream file(physicalPath, std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	if (!data.empty())
	{
		file.write(reinterpret_cast<char const*>(data.data()), data.size());
	}

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
	std::unordered_map<VirtualPath, std::filesystem::file_time_type> importedTimes;
	ScanResourceImports(importedTimes);

	// 2) Move the previous tree aside so import times can be copied into the new tree.
	std::unique_ptr<FileEntry> previousRootEntry = std::move(m_rootEntry);

	// 3) Rebuild the visible resource tree, hiding generated import metadata/cache entries.
	m_rootEntry = std::unique_ptr<FileEntry>(
		new FileEntry(m_resourceRoot, VirtualPath::ResourceRoot(), "res://", "res://", true, nullptr));
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

		std::string const          name             = GetDisplayName(entry.path());
		VirtualPath const          childVirtualPath = VirtualPath::ResourceRoot().Join(name);
		std::unique_ptr<FileEntry> childEntry       = BuildEntry(
			entry.path(),
			childVirtualPath,
			m_rootEntry.get(),
			isDirectory,
			FindEntryInTree(previousRootEntry.get(), childVirtualPath),
			importedTimes);
		if (childEntry && (childEntry->IsDirectory() || IsVisibleResourceFile(childEntry->GetVirtualPath())))
		{
			m_rootEntry->m_children.push_back(std::move(childEntry));
		}
	}

	SortChildren(*m_rootEntry);
}

bool FileSystem::HasResourceTree() const { return m_rootEntry != nullptr; }

FileEntry const* FileSystem::GetResourceRootEntry() const { return m_rootEntry.get(); }

bool FileSystem::TryToVirtualPath(std::filesystem::path const& physicalPath, VirtualPath& outVirtualPath) const
{
	outVirtualPath = {};
	std::error_code errorCode;
	std::filesystem::path const canonicalRoot = std::filesystem::weakly_canonical(m_resourceRoot, errorCode);
	if (errorCode)
	{
		return false;
	}
	std::filesystem::path const canonicalPath = std::filesystem::weakly_canonical(physicalPath, errorCode);
	if (errorCode)
	{
		return false;
	}
	std::filesystem::path const relativePath = std::filesystem::relative(canonicalPath, canonicalRoot, errorCode);
	if (errorCode || (!relativePath.empty() && relativePath.begin() != relativePath.end() && *relativePath.begin() == ".."))
	{
		return false;
	}
	if (relativePath.empty() || relativePath == ".")
	{
		outVirtualPath = VirtualPath::ResourceRoot();
		return true;
	}
	return VirtualPath::TryParse("res://" + relativePath.generic_string(), outVirtualPath);
}

bool FileSystem::TryGetPhysicalPath(VirtualPath const& virtualPath, std::filesystem::path& outPhysicalPath) const
{
	outPhysicalPath.clear();
	if (!virtualPath.IsValid())
	{
		return false;
	}

	std::string const relativePath = virtualPath.GetString().substr(std::string("res://").size());
	std::error_code errorCode;
	std::filesystem::path const canonicalRoot = std::filesystem::weakly_canonical(m_resourceRoot, errorCode);
	if (errorCode)
	{
		return false;
	}
	std::filesystem::path const candidate = relativePath.empty() ? canonicalRoot : canonicalRoot / relativePath;
	std::filesystem::path const canonicalPath = std::filesystem::weakly_canonical(candidate, errorCode);
	if (errorCode)
	{
		return false;
	}
	VirtualPath roundTrip;
	if (!TryToVirtualPath(canonicalPath, roundTrip) || roundTrip != virtualPath)
	{
		return false;
	}
	outPhysicalPath = canonicalPath;
	return true;
}

bool FileSystem::TryGetWritablePhysicalPath(
	VirtualPath const&       virtualPath,
	std::filesystem::path&   outPhysicalPath,
	std::string&             outError,
	bool                     allowResourceRoot) const
{
	outPhysicalPath.clear();
	outError.clear();

	if (virtualPath.IsRoot())
	{
		if (!allowResourceRoot)
		{
			outError = "The resource root cannot be modified.";
			return false;
		}
		outPhysicalPath = m_resourceRoot;
	}
	else if (!TryGetPhysicalPath(virtualPath, outPhysicalPath))
	{
		outError = "Virtual path is invalid.";
		return false;
	}

	std::error_code errorCode;
	std::filesystem::path const canonicalRoot = std::filesystem::weakly_canonical(m_resourceRoot, errorCode);
	if (errorCode)
	{
		outError = JoinError("Failed to resolve resource root", errorCode);
		return false;
	}
	std::filesystem::path const canonicalPath = std::filesystem::weakly_canonical(outPhysicalPath, errorCode);
	if (errorCode)
	{
		outError = JoinError("Failed to resolve path", errorCode);
		return false;
	}

	auto rootPart = canonicalRoot.begin();
	auto pathPart = canonicalPath.begin();
	for (; rootPart != canonicalRoot.end() && pathPart != canonicalPath.end(); ++rootPart, ++pathPart)
	{
		if (*rootPart != *pathPart)
		{
			outError = "Path is outside the resource root.";
			return false;
		}
	}
	if (rootPart != canonicalRoot.end())
	{
		outError = "Path is outside the resource root.";
		return false;
	}

	outPhysicalPath = canonicalPath;
	return true;
}

FileEntry const* FileSystem::FindEntry(VirtualPath const& virtualPath) const
{
	return FindEntryInTree(m_rootEntry.get(), virtualPath);
}

void FileSystem::ScanResourceImports(std::unordered_map<VirtualPath, std::filesystem::file_time_type>& outImportedTimes)
{
	std::error_code errorCode;
	for (std::filesystem::recursive_directory_iterator it(m_resourceRoot, errorCode), end; it != end;
		 it.increment(errorCode))
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

		VirtualPath virtualPath;
		if (!TryToVirtualPath(entry.path(), virtualPath) || !ResourceImporter::CanImport(virtualPath))
		{
			continue;
		}

		// 1) Read the current source modified time from the filesystem.
		std::filesystem::file_time_type modifiedTime;
		if (!TryGetLastWriteTime(entry.path(), modifiedTime))
		{
			continue;
		}

		// 2) Let ResourceImporter keep the config/cache chain complete.
		if (ResourceImporter::EnsureImported(virtualPath))
		{
			outImportedTimes[virtualPath] = modifiedTime;
		}
	}
}

std::unique_ptr<FileEntry> FileSystem::BuildEntry(
	std::filesystem::path const&                                            physicalPath,
	VirtualPath const&                                                      virtualPath,
	FileEntry*                                                              parent,
	bool                                                                    isDirectory,
	FileEntry const*                                                        previousEntry,
	std::unordered_map<VirtualPath, std::filesystem::file_time_type> const& importedTimes) const
{
	if (ShouldSkipResourceTreeEntry(physicalPath, isDirectory))
	{
		return nullptr;
	}

	if (!isDirectory && !IsVisibleResourceFile(virtualPath))
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

			std::string const          childName        = GetDisplayName(child.path());
			VirtualPath const          childVirtualPath = virtualPath.Join(childName);
			std::unique_ptr<FileEntry> childEntry       = BuildEntry(
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

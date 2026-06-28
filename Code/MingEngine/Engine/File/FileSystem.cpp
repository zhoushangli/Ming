#include "MingEngine/Engine/File/FileSystem.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{
constexpr char const* kResourcePathPrefix = "res://";

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

FileSystem::FileSystem(FileSystemConfig const& config) : m_resourceRoot(config.m_resourceRoot) {}

void FileSystem::BindMethods() {}

void FileSystem::Startup() {}
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
	m_rootEntry.reset();

	std::error_code errorCode;
	if (!std::filesystem::exists(m_resourceRoot, errorCode)
		|| !std::filesystem::is_directory(m_resourceRoot, errorCode))
	{
		return;
	}

	m_rootEntry =
		std::unique_ptr<FileEntry>(
			new FileEntry(m_resourceRoot, kResourcePathPrefix, kResourcePathPrefix, kResourcePathPrefix, true, nullptr));

	for (std::filesystem::directory_entry const& entry : std::filesystem::directory_iterator(m_resourceRoot, errorCode))
	{
		std::error_code entryError;
		bool const      isDirectory = entry.is_directory(entryError);
		bool const      isFile      = entry.is_regular_file(entryError);
		if (entryError || (!isDirectory && !isFile))
		{
			continue;
		}

		std::string const name = GetDisplayName(entry.path());
		m_rootEntry->m_children.push_back(
			BuildEntry(entry.path(), JoinVirtualPath(kResourcePathPrefix, name), m_rootEntry.get(), isDirectory));
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

std::unique_ptr<FileEntry> FileSystem::BuildEntry(
	std::filesystem::path const& physicalPath,
	std::string const&           virtualPath,
	FileEntry*                   parent,
	bool                         isDirectory) const
{
	std::string const          name = GetDisplayName(physicalPath);
	std::unique_ptr<FileEntry> result(
		new FileEntry(physicalPath, virtualPath, name, ToLower(name), isDirectory, parent));

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

			std::string const childName = GetDisplayName(child.path());
			result->m_children.push_back(
				BuildEntry(child.path(), JoinVirtualPath(virtualPath, childName), result.get(), childIsDirectory));
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

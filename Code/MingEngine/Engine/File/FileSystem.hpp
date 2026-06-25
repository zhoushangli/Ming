#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct FileSystemConfig
{
	bool                  m_isEnable     = true;
	std::filesystem::path m_resourceRoot = "Data/";
};

class FileEntry
{
	friend class FileSystem;

public:
	FileEntry(FileEntry const&)            = delete;
	FileEntry& operator=(FileEntry const&) = delete;
	FileEntry(FileEntry&&)                 = delete;
	FileEntry& operator=(FileEntry&&)      = delete;
	~FileEntry()                           = default;

	std::filesystem::path const&                   GetPhysicalPath() const;
	std::string const&                             GetVirtualPath() const;
	std::string const&                             GetName() const;
	std::string const&                             GetLowerName() const;
	bool                                           IsDirectory() const;
	FileEntry const*                               GetParent() const;
	std::vector<std::unique_ptr<FileEntry>> const& GetChildren() const;

private:
	FileEntry(
		std::filesystem::path physicalPath,
		std::string           virtualPath,
		std::string           name,
		std::string           lowerName,
		bool                  isDirectory,
		FileEntry*            parent);

private:
	std::filesystem::path                   m_physicalPath;
	std::string                             m_virtualPath;
	std::string                             m_name;
	std::string                             m_lowerName;
	bool                                    m_isDirectory = false;
	FileEntry*                              m_parent      = nullptr;
	std::vector<std::unique_ptr<FileEntry>> m_children;
};

class FileSystem
{
public:
	FileSystem(FileSystemConfig const& config);

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	bool Exists(VirtualPath const& virtualPath) const;

	bool ReadText(VirtualPath const& virtualPath, std::string& outText) const;

	std::filesystem::path const& GetResourceRoot() const;
	void                         ScanResourceTree();
	bool                         HasResourceTree() const;
	FileEntry const*             GetResourceRootEntry() const;
	std::string                  ToVirtualPath(std::filesystem::path const& physicalPath) const;

private:
	bool ResolvePath(VirtualPath const& virtualPath, std::filesystem::path& outPhysicalPath) const;
	std::unique_ptr<FileEntry> BuildEntry(
		std::filesystem::path const& physicalPath,
		std::string const&           virtualPath,
		FileEntry*                   parent,
		bool                         isDirectory) const;
	void SortChildren(FileEntry& entry) const;

private:
	std::filesystem::path      m_resourceRoot;
	std::unique_ptr<FileEntry> m_rootEntry;
};

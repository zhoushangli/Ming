#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

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

class FileSystem : public SystemBase
{
	MCLASS(FileSystem, SystemBase)

public:
	FileSystem(FileSystemConfig const& config);

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	static bool IsVirtualPath(std::string const& path);
	static bool TryGetRelativePath(std::string const& virtualPath, std::string& outRelativePath);

	bool Exists(std::string const& virtualPath) const;

	bool ReadText(std::string const& virtualPath, std::string& outText) const;
	bool WriteText(std::string const& virtualPath, std::string const& text) const;

	std::filesystem::path const& GetResourceRoot() const;
	void                         ScanResourceTree();
	bool                         HasResourceTree() const;
	FileEntry const*             GetResourceRootEntry() const;
	std::string                  ToVirtualPath(std::filesystem::path const& physicalPath) const;

	static void BindMethods();

private:
	bool ResolvePath(std::string const& virtualPath, std::filesystem::path& outPhysicalPath) const;
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

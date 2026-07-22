#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
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
	VirtualPath const&                             GetVirtualPath() const;
	std::string const&                             GetName() const;
	std::string const&                             GetLowerName() const;
	bool                                           IsDirectory() const;
	FileEntry const*                               GetParent() const;
	std::vector<std::unique_ptr<FileEntry>> const& GetChildren() const;
	bool                                           HasModifiedTime() const;
	bool                                           HasImportTime() const;
	std::filesystem::file_time_type                GetModifiedTime() const;
	std::filesystem::file_time_type                GetImportTime() const;

private:
	FileEntry(
		std::filesystem::path physicalPath,
		VirtualPath           virtualPath,
		std::string           name,
		std::string           lowerName,
		bool                  isDirectory,
		FileEntry*            parent);

private:
	std::filesystem::path                   m_physicalPath;
	VirtualPath                             m_virtualPath;
	std::string                             m_name;
	std::string                             m_lowerName;
	bool                                    m_isDirectory     = false;
	FileEntry*                              m_parent          = nullptr;
	bool                                    m_hasModifiedTime = false;
	bool                                    m_hasImportTime   = false;
	std::filesystem::file_time_type         m_modifiedTime;
	std::filesystem::file_time_type         m_importTime;
	std::vector<std::unique_ptr<FileEntry>> m_children;
};

// For importable resources, we will import them into the .ming folder as engine inner type
// and leave a .import file in the original location to point to the imported resource
class FileSystem : public SystemBase
{
	MCLASS(FileSystem, SystemBase)

public:
	FileSystem(FileSystemConfig const& config);

	void Startup() override;

	bool ReadText(VirtualPath const& virtualPath, std::string& outText) const;
	bool WriteText(VirtualPath const& virtualPath, std::string const& text) const;
	bool ReadBinary(VirtualPath const& virtualPath, std::vector<uint8_t>& outData) const;
	bool WriteBinary(VirtualPath const& virtualPath, std::vector<uint8_t> const& data) const;
	bool CreateFolder(
		VirtualPath const& parentVirtualPath,
		std::string const& name,
		VirtualPath&       outVirtualPath,
		std::string&       outError) const;
	bool Rename(
		VirtualPath const& virtualPath,
		std::string const& newName,
		VirtualPath&       outVirtualPath,
		std::string&       outError) const;
	bool Duplicate(VirtualPath const& virtualPath, VirtualPath& outVirtualPath, std::string& outError) const;
	bool Remove(VirtualPath const& virtualPath, std::string& outError) const;
	bool Move(
		VirtualPath const& sourceVirtualPath,
		VirtualPath const& targetDirectoryVirtualPath,
		VirtualPath&       outVirtualPath,
		std::string&       outError) const;

	std::filesystem::path const& GetResourceRoot() const;
	void                         ScanResourceTree();
	bool                         HasResourceTree() const;
	FileEntry const*             GetResourceRootEntry() const;

	bool Exists(VirtualPath const& virtualPath) const;
	bool TryToVirtualPath(std::filesystem::path const& physicalPath, VirtualPath& outVirtualPath) const;
	bool TryGetPhysicalPath(VirtualPath const& virtualPath, std::filesystem::path& outPhysicalPath) const;

	static void BindMethods();

private:
	void ScanResourceImports(std::unordered_map<VirtualPath, std::filesystem::file_time_type>& outImportedTimes);
	FileEntry const*           FindEntry(VirtualPath const& virtualPath) const;
	std::unique_ptr<FileEntry> BuildEntry(
		std::filesystem::path const&                                            physicalPath,
		VirtualPath const&                                                      virtualPath,
		FileEntry*                                                              parent,
		bool                                                                    isDirectory,
		FileEntry const*                                                        previousEntry,
		std::unordered_map<VirtualPath, std::filesystem::file_time_type> const& importedTimes) const;
	void SortChildren(FileEntry& entry) const;

private:
	std::filesystem::path      m_resourceRoot;
	std::unique_ptr<FileEntry> m_rootEntry;
};

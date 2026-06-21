#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <filesystem>
#include <string>

struct FileSystemConfig
{
	bool                  m_isEnable     = true;
	std::filesystem::path m_resourceRoot = "Data/";
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

private:
	bool ResolvePath(VirtualPath const& virtualPath, std::filesystem::path& outPhysicalPath) const;

private:
	std::filesystem::path m_resourceRoot;
};

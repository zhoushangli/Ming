#pragma once

#include <filesystem>
#include <string>

class FileSystem
{
public:
	explicit FileSystem(std::filesystem::path const& resourceRoot);

	bool Exists(std::string const& virtualPath) const;

	bool ReadText(std::string const& virtualPath, std::string& outText) const;

private:
	bool ResolvePath(std::string const& virtualPath, std::filesystem::path& outPhysicalPath) const;

private:
	std::filesystem::path m_resourceRoot;
};
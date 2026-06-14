#include "MingEngine/Engine/File/FileSystem.hpp"

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <fstream>
#include <sstream>

FileSystem::FileSystem(FileSystemConfig const& config) : m_resourceRoot(config.m_resourceRoot) {}

void FileSystem::Startup() {}
void FileSystem::Shutdown() {}
void FileSystem::BeginFrame() {}
void FileSystem::EndFrame() {}

bool FileSystem::Exists(VirtualPath const& virtualPath) const
{
	std::filesystem::path physicalPath;
	if (!ResolvePath(virtualPath, physicalPath))
	{
		return false;
	}

	return std::filesystem::exists(physicalPath) && std::filesystem::is_regular_file(physicalPath);
}

bool FileSystem::ReadText(VirtualPath const& virtualPath, std::string& outText) const
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

bool FileSystem::ResolvePath(VirtualPath const& virtualPath, std::filesystem::path& outPhysicalPath) const
{
	outPhysicalPath.clear();

	outPhysicalPath = m_resourceRoot / virtualPath.GetRelativePath();

	return true;
}

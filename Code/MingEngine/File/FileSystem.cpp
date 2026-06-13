#include "MingEngine/File/FileSystem.hpp"

#include "MingEngine/File/VirtualPath.hpp"

#include <fstream>
#include <sstream>

FileSystem::FileSystem(std::filesystem::path const& resourceRoot) : m_resourceRoot(resourceRoot) {}

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

bool FileSystem::ResolvePath(std::string const& virtualPath, std::filesystem::path& outPhysicalPath) const
{
	outPhysicalPath.clear();

	VirtualPath parsedPath;

	if (!parsedPath.Parse(virtualPath))
	{
		return false;
	}

	outPhysicalPath = m_resourceRoot / parsedPath.GetRelativePath();

	return true;
}

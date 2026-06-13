#include "MingEngine/File/VirtualPath.hpp"

bool VirtualPath::Parse(std::string const& path)
{
	m_relativePath.clear();

	if (path.empty())
	{
		return false;
	}

	std::string const prefix = "res://";

	if (path.compare(0, prefix.size(), prefix) != 0)
	{
		return false;
	}

	m_relativePath = path.substr(prefix.size());

	if (m_relativePath.empty())
	{
		return false;
	}

	if (m_relativePath.find('\\') != std::string::npos)
	{
		return false;
	}

	if (m_relativePath == ".." || m_relativePath.starts_with("../") || m_relativePath.find("/../") != std::string::npos
		|| m_relativePath.ends_with("/.."))
	{
		return false;
	}

	return true;
}

std::string const& VirtualPath::GetRelativePath() const { return m_relativePath; }

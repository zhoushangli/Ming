#include "MingEngine/Engine/File/VirtualPath.hpp"

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

	if (m_relativePath == ".." || m_relativePath.compare(0, 3, "../") == 0
		|| m_relativePath.find("/../") != std::string::npos
		|| (m_relativePath.size() >= 3 && m_relativePath.compare(m_relativePath.size() - 3, 3, "/..") == 0))
	{
		return false;
	}

	return true;
}

std::string VirtualPath::GetVirtualPath() const { return ToString(); }

std::string const& VirtualPath::GetRelativePath() const { return m_relativePath; }

bool VirtualPath::operator==(VirtualPath const& other) const { return m_relativePath == other.m_relativePath; }

std::string VirtualPath::ToString() const { return "res://" + m_relativePath; }
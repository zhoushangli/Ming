#include "ScriptResourceIdentity.hpp"
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

	if (m_relativePath == ".." || m_relativePath.starts_with("../") || m_relativePath.find("/../") != std::string::npos
		|| m_relativePath.ends_with("/.."))
	{
		return false;
	}

	return true;
}

std::string const& VirtualPath::GetRelativePath() const { return m_relativePath; }

bool MakeScriptResourceIdentity(std::string const& scriptPath, ScriptResourceIdentity& outIdentity)
{
	VirtualPath vp;
	if (!vp.Parse(scriptPath))
	{
		return false;
	}

	// Wheather the file extension is ".as"
	std::string const& relativePath = vp.GetRelativePath();
	if (relativePath.size() < 3 || relativePath.compare(relativePath.size() - 3, 3, ".as") != 0)
	{
		return false;
	}

	// Get indentity
	size_t const fileNameBegin  = relativePath.find_last_of('/');
	size_t const fileNameEnd = relativePath.size() - 3; // Exclude ".as" extension
	outIdentity.m_path = scriptPath;
	outIdentity.m_className = relativePath.substr(fileNameBegin + 1, fileNameEnd - fileNameBegin - 1);

	return true;
}
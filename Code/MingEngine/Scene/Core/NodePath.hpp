#pragma once

#include <string>
#include <utility>
#include <vector>

class NodePath
{
public:
	NodePath() = default;
	explicit NodePath(std::string path);

	bool IsValid() const { return !m_paths.empty(); }
	bool IsAbsolute() const { return m_isAbsolute; }
	bool IsCurrent() const { return !m_isAbsolute && m_paths.size() == 1 && m_paths[0] == "."; }

	std::vector<std::string> const& GetPaths() const { return m_paths; }

protected:
	void Parse(std::string path);

	std::vector<std::string> m_paths;
	bool                     m_isAbsolute = false;
};

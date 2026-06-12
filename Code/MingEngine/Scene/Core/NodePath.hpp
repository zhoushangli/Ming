#pragma once

#include <string>
#include <utility>
#include <vector>

class NodePath
{
public:
	NodePath() = default;
	explicit NodePath(std::string path);

	bool IsValid() const;
	bool IsAbsolute() const;
	bool IsCurrent() const;

	std::vector<std::string> const& GetPaths() const;

protected:
	void Parse(std::string path);

	std::vector<std::string> m_paths;
	bool                     m_isAbsolute = false;
};

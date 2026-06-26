#pragma once

#include <string>

class VirtualPath
{
public:
	// Wrong cases:
	// 1) Empty path
	// 2) res://
	// 3) res://../Secret.txt
	// 4) res://Assets/../Secret.txt
	// 5) C:/Game/Test.as
	bool Parse(std::string const& path);

	std::string GetVirtualPath() const;
	std::string const& GetRelativePath() const;

	bool        operator==(VirtualPath const& other) const;
	std::string ToString() const;

private:
	std::string m_relativePath;
};

struct VirtualPathHash
{
	size_t operator()(VirtualPath const& path) const noexcept
	{
		return std::hash<std::string>{}(path.GetRelativePath());
	}
};
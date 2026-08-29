#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

class VirtualPath
{
public:
	VirtualPath() = default;
	VirtualPath(char const* path);
	VirtualPath(std::string const& path);

	static bool        TryParse(std::string_view path, VirtualPath& outPath);
	static VirtualPath ResourceRoot();

	bool IsValid() const;
	bool IsRoot() const;

	VirtualPath GetParent() const;
	std::string GetFileName() const;
	std::string GetStem() const;
	std::string GetExtension() const;
	bool        HasExtension(std::string_view extension) const;
	VirtualPath Join(std::string_view child) const;
	bool        TryResolveRelative(std::string_view relativePath, VirtualPath& outPath) const;

	std::string const& GetString() const;
	char const*        CStr() const;

	bool operator==(VirtualPath const& other) const;
	bool operator!=(VirtualPath const& other) const;

private:
	explicit VirtualPath(std::string path, bool isValidated);
	static bool TryParseInternal(std::string_view path, std::string& outPath);

private:
	std::string m_path;
};

namespace std
{
template <>
struct hash<VirtualPath>
{
	size_t operator()(VirtualPath const& path) const noexcept
	{
		return hash<std::string>{}(path.GetString());
	}
};
}

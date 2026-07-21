#include "MingEngine/Engine/File/VirtualPath.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace
{
constexpr std::string_view kResourcePrefix = "res://";

void ReportInvalidVirtualPath(char const* path)
{
	DebuggerPrintf("VirtualPath: invalid path '%s'.\n", path != nullptr ? path : "<null>");
}

std::string ToLower(std::string_view text)
{
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char value) {
		return static_cast<char>(std::tolower(value));
	});
	return result;
}
}

VirtualPath::VirtualPath(char const* path)
{
	if (path == nullptr || !TryParseInternal(path, m_path))
	{
		ReportInvalidVirtualPath(path);
		m_path.clear();
	}
}

VirtualPath::VirtualPath(std::string const& path)
	: VirtualPath(path.c_str())
{
}

VirtualPath::VirtualPath(std::string path, bool)
	: m_path(std::move(path))
{
}

bool VirtualPath::TryParse(std::string_view path, VirtualPath& outPath)
{
	std::string parsedPath;
	if (!TryParseInternal(path, parsedPath))
	{
		outPath = {};
		return false;
	}

	outPath = VirtualPath(std::move(parsedPath), true);
	return true;
}

VirtualPath VirtualPath::ResourceRoot()
{
	return VirtualPath(std::string(kResourcePrefix), true);
}

bool VirtualPath::IsValid() const
{
	return !m_path.empty();
}

bool VirtualPath::IsRoot() const
{
	return m_path == kResourcePrefix;
}

VirtualPath VirtualPath::GetParent() const
{
	if (!IsValid() || IsRoot())
	{
		return {};
	}

	size_t const separator = m_path.find_last_of('/');
	if (separator < kResourcePrefix.size())
	{
		return ResourceRoot();
	}
	return VirtualPath(m_path.substr(0, separator), true);
}

std::string VirtualPath::GetFileName() const
{
	if (!IsValid() || IsRoot())
	{
		return {};
	}
	return m_path.substr(m_path.find_last_of('/') + 1);
}

std::string VirtualPath::GetStem() const
{
	std::string const fileName = GetFileName();
	size_t const      extension = fileName.find_last_of('.');
	if (extension == std::string::npos || extension == 0)
	{
		return fileName;
	}
	return fileName.substr(0, extension);
}

std::string VirtualPath::GetExtension() const
{
	std::string const fileName = GetFileName();
	size_t const      extension = fileName.find_last_of('.');
	if (extension == std::string::npos || extension == 0 || extension + 1 == fileName.size())
	{
		return {};
	}
	return ToLower(std::string_view(fileName).substr(extension));
}

bool VirtualPath::HasExtension(std::string_view extension) const
{
	if (!extension.empty() && extension.front() != '.')
	{
		std::string dottedExtension = ".";
		dottedExtension.append(extension);
		return GetExtension() == ToLower(dottedExtension);
	}
	return GetExtension() == ToLower(extension);
}

VirtualPath VirtualPath::Join(std::string_view child) const
{
	if (!IsValid() || child.empty() || child.front() == '/' || child.back() == '/')
	{
		DebuggerPrintf("VirtualPath::Join: invalid child '%.*s'.\n", static_cast<int>(child.size()), child.data());
		return {};
	}

	std::string joined = m_path;
	if (!IsRoot())
	{
		joined.push_back('/');
	}
	joined.append(child);

	VirtualPath result;
	if (!TryParse(joined, result))
	{
		DebuggerPrintf("VirtualPath::Join: invalid child '%.*s'.\n", static_cast<int>(child.size()), child.data());
	}
	return result;
}

bool VirtualPath::TryResolveRelative(std::string_view relativePath, VirtualPath& outPath) const
{
	outPath = {};
	if (!IsValid() || relativePath.empty())
	{
		return false;
	}

	std::string normalized(relativePath);
	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	if (normalized.rfind(kResourcePrefix, 0) == 0)
	{
		return TryParse(normalized, outPath);
	}
	if (normalized.front() == '/')
	{
		return false;
	}

	std::vector<std::string> segments;
	VirtualPath const        base = IsRoot() ? *this : GetParent();
	if (!base.IsValid())
	{
		return false;
	}

	std::string_view baseRelative(base.GetString().data() + kResourcePrefix.size(),
		base.GetString().size() - kResourcePrefix.size());
	size_t position = 0;
	while (position < baseRelative.size())
	{
		size_t const separator = baseRelative.find('/', position);
		segments.emplace_back(baseRelative.substr(position, separator - position));
		position = separator == std::string_view::npos ? baseRelative.size() : separator + 1;
	}

	position = 0;
	while (position <= normalized.size())
	{
		size_t const separator = normalized.find('/', position);
		std::string_view const segment(normalized.data() + position,
			(separator == std::string::npos ? normalized.size() : separator) - position);
		if (segment.empty() || segment == ".")
		{
			// Ignore external-format separators and current-directory segments.
		}
		else if (segment == "..")
		{
			if (segments.empty())
			{
				return false;
			}
			segments.pop_back();
		}
		else
		{
			segments.emplace_back(segment);
		}

		if (separator == std::string::npos)
		{
			break;
		}
		position = separator + 1;
	}

	std::string resolved(kResourcePrefix);
	for (size_t index = 0; index < segments.size(); ++index)
	{
		if (index > 0)
		{
			resolved.push_back('/');
		}
		resolved.append(segments[index]);
	}
	return TryParse(resolved, outPath);
}

std::string const& VirtualPath::GetString() const
{
	return m_path;
}

char const* VirtualPath::CStr() const
{
	return m_path.c_str();
}

bool VirtualPath::operator==(VirtualPath const& other) const
{
	return m_path == other.m_path;
}

bool VirtualPath::operator!=(VirtualPath const& other) const
{
	return !(*this == other);
}

bool VirtualPath::TryParseInternal(std::string_view path, std::string& outPath)
{
	outPath.clear();
	if (path.empty() || path.rfind(kResourcePrefix, 0) != 0 || path.find('\\') != std::string_view::npos)
	{
		return false;
	}
	if (path == kResourcePrefix)
	{
		outPath.assign(path);
		return true;
	}
	if (path.back() == '/')
	{
		return false;
	}

	std::string_view const relative = path.substr(kResourcePrefix.size());
	size_t                 position = 0;
	while (position < relative.size())
	{
		size_t const separator = relative.find('/', position);
		std::string_view const segment = relative.substr(position, separator - position);
		if (segment.empty() || segment == "." || segment == "..")
		{
			return false;
		}
		position = separator == std::string_view::npos ? relative.size() : separator + 1;
	}

	outPath.assign(path);
	return true;
}

#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <utility>

void Resource::BindMethods()
{
	ClassDatabase::BindMethod("GetPath", &Resource::GetPathString, {});
	ClassDatabase::BindMethod("SetPath", &Resource::SetPathString, { "path" });
	ClassDatabase::BindMethod("GetName", &Resource::GetName, {});
	ClassDatabase::BindMethod("SetName", &Resource::SetName, { "name" });

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::String, "path", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default),
		"SetPath",
		"GetPath");
}

String Resource::GetPathString() const { return String(m_virtualPath.GetString()); }

void Resource::SetPathString(String const& path)
{
	if (path.IsEmpty())
	{
		m_virtualPath = {};
		return;
	}

	// 1) The virtual path parser works on UTF-8 text
	// 2) Keep the text for the error message as well
	std::string const pathText = path.ToUtf8();

	VirtualPath parsedPath;
	ERR_FAIL_COND_MSG(
		!VirtualPath::TryParse(pathText, parsedPath),
		Stringf("Invalid virtual path '%s'.", pathText.c_str()));
	m_virtualPath = std::move(parsedPath);
}

void Resource::MoveBaseFrom(Resource&& other)
{
	m_name           = std::move(other.m_name);
	m_virtualPath    = std::move(other.m_virtualPath);
	m_sourceFilePath = std::move(other.m_sourceFilePath);
}

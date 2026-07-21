#include "MingEngine/Core/Object/Resource.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <utility>

void Resource::BindMethods()
{
	ClassDatabase::BindMethod("GetPath", &Resource::GetPathString);
	ClassDatabase::BindMethod("SetPath", &Resource::SetPathString);
	ClassDatabase::BindMethod("GetName", &Resource::GetName);
	ClassDatabase::BindMethod("SetName", &Resource::SetName);

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::String, "path", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default),
		"SetPath",
		"GetPath");
}

std::string Resource::GetPathString() const
{
	return m_virtualPath.GetString();
}

void Resource::SetPathString(std::string const& path)
{
	if (path.empty())
	{
		m_virtualPath = {};
		return;
	}

	VirtualPath parsedPath;
	if (VirtualPath::TryParse(path, parsedPath))
	{
		m_virtualPath = std::move(parsedPath);
	}
	else
	{
		DebuggerPrintf("Resource::SetPathString: invalid virtual path '%s'.\n", path.c_str());
	}
}

void Resource::MoveBaseFrom(Resource&& other)
{
	m_name           = std::move(other.m_name);
	m_virtualPath    = std::move(other.m_virtualPath);
	m_sourceFilePath = std::move(other.m_sourceFilePath);
}

#include "MingEngine/Core/Object/Resource.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <utility>

void Resource::BindMethods()
{
	ClassDatabase::BindMethod("GetPath", &Resource::GetVirtualPath);
	ClassDatabase::BindMethod("SetPath", &Resource::SetVirtualPath);
	ClassDatabase::BindMethod("GetName", &Resource::GetName);
	ClassDatabase::BindMethod("SetName", &Resource::SetName);

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::String, "path", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default),
		"SetPath",
		"GetPath");
}

void Resource::MoveBaseFrom(Resource&& other)
{
	m_name           = std::move(other.m_name);
	m_virtualPath    = std::move(other.m_virtualPath);
	m_sourceFilePath = std::move(other.m_sourceFilePath);
}

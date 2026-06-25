#include "MingEngine/Core/Object/Resource.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

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

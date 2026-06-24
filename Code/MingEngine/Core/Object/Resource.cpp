#include "MingEngine/Core/Object/Resource.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

void Resource::BindMethods()
{
	ClassDatabase::BindMethod("GetPath", &Resource::GetPath);
	ClassDatabase::BindMethod("SetPath", &Resource::SetPath);

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::String, "path", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default),
		"SetPath",
		"GetPath");
}

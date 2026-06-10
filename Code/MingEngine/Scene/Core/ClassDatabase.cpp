#include "MingEngine/Scene/Core/ClassDatabase.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"
#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"

#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"

std::unordered_map<std::string, ClassInfo> ClassDatabase::m_classInfoMap;

void ClassDatabase::Startup()
{
	m_classInfoMap.clear();

	RegisterClass<Node>();
	RegisterClass<Node3D>();
	RegisterClass<Camera3D>();
	RegisterClass<Light3D>(false);
	RegisterClass<DirectionalLight3D>();
	RegisterClass<PointLight3D>();
	RegisterClass<Collider3D>(false);
	RegisterClass<AABBCollider3D>();
	RegisterClass<CapsuleCollider3D>();
	RegisterClass<CylinderZCollider3D>();
	RegisterClass<TriangleMeshCollider3D>();
}

void ClassDatabase::Shutdown() { m_classInfoMap.clear(); }

Object* ClassDatabase::CreateInstance(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return nullptr;
	}

	ClassInfo const& classInfo = iter->second;
	if (!classInfo.m_creator)
	{
		return nullptr;
	}
	return classInfo.m_creator();
}

ClassInfo const* ClassDatabase::GetClassInfo(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return nullptr;
	}

	return &iter->second;
}

std::vector<ClassInfo const*> ClassDatabase::GetRegisteredClasses()
{
	std::vector<ClassInfo const*> classes;
	classes.reserve(m_classInfoMap.size());
	for (auto const& classEntry : m_classInfoMap)
	{
		classes.push_back(&classEntry.second);
	}
	return classes;
}

bool ClassDatabase::IsSubclassOf(std::string const& className, std::string const& baseClassName)
{
	std::string currentClassName = className;
	for (size_t depth = 0; depth <= m_classInfoMap.size(); ++depth)
	{
		if (currentClassName == baseClassName)
		{
			return true;
		}

		ClassInfo const* classInfo = GetClassInfo(currentClassName);
		if (classInfo == nullptr || classInfo->m_parentClassName == currentClassName)
		{
			return false;
		}
		currentClassName = classInfo->m_parentClassName;
	}

	return false;
}

std::vector<PropertyInfo> ClassDatabase::GetProperties(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return {};
	}

	return iter->second.m_properties;
}

std::vector<PropertyInfo const*> ClassDatabase::GetAllProperties(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return {};
	}

	std::vector<PropertyInfo const*> properties = GetAllProperties(iter->second.m_parentClassName);
	for (PropertyInfo const& property : iter->second.m_properties)
	{
		properties.push_back(&property);
	}

	return properties;
}

PropertyInfo const* ClassDatabase::FindProperty(std::string const& className, std::string const& propertyName)
{
	std::vector<PropertyInfo const*> properties = GetAllProperties(className);
	for (auto iter = properties.rbegin(); iter != properties.rend(); ++iter)
	{
		PropertyInfo const* property = *iter;
		if (property != nullptr && property->m_name == propertyName)
		{
			return property;
		}
	}

	return nullptr;
}

MethodBind const* ClassDatabase::GetMethodBind(std::string const& className, std::string const& methodName)
{
	ClassInfo const* classInfo = GetClassInfo(className);
	if (classInfo == nullptr)
	{
		return nullptr;
	}

	for (auto const& method : classInfo->m_methods)
	{
		if (method.m_name == methodName)
		{
			return method.m_bind.get();
		}
	}

	return nullptr;
}

void ClassDatabase::AddProperty(
	std::string const& className,
	PropertyInfo propertyInfo,
	std::string const& setterName,
	std::string const& getterName)
{
	propertyInfo.m_setterName = setterName;
	propertyInfo.m_getterName = getterName;
	propertyInfo.m_setter     = GetMethodBind(className, setterName);
	propertyInfo.m_getter     = GetMethodBind(className, getterName);

	GUARANTEE_OR_DIE(
		propertyInfo.m_setter != nullptr,
		Stringf("ClassDatabase: setter '%s' is not bound on class '%s'.", setterName.c_str(), className.c_str()));
	GUARANTEE_OR_DIE(
		propertyInfo.m_getter != nullptr,
		Stringf("ClassDatabase: getter '%s' is not bound on class '%s'.", getterName.c_str(), className.c_str()));

	m_classInfoMap[className].m_properties.push_back(std::move(propertyInfo));
}

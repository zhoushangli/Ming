#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <algorithm>

std::unordered_map<std::string, ClassInfo>           ClassDatabase::m_classInfoMap;
std::unordered_map<std::string, GlobalNamespaceInfo> ClassDatabase::m_namespaceInfoMap;
std::unordered_map<std::string, Object*>             ClassDatabase::m_globalObjectMap;
ApiType                                              ClassDatabase::m_currentApiType = ApiType::None;

namespace
{
int GetClassInheritanceDepth(ClassInfo const& classInfo, size_t maxDepth)
{
	int         depth           = 0;
	std::string parentClassName = classInfo.m_parentClassName;

	for (size_t step = 0; !parentClassName.empty() && step <= maxDepth; ++step)
	{
		ClassInfo const* parentInfo = ClassDatabase::GetClassInfo(parentClassName);
		if (parentInfo == nullptr || parentInfo->m_className == parentInfo->m_parentClassName)
		{
			break;
		}

		++depth;
		parentClassName = parentInfo->m_parentClassName;
	}

	return depth;
}
} // namespace

void ClassDatabase::Startup() { m_classInfoMap.clear(); }

void ClassDatabase::Shutdown() { m_classInfoMap.clear(); }

Object* ClassDatabase::CreateInstance(std::string const& className)
{
	ConstructorFunc constructor = GetConstructor(className);
	if (!constructor)
	{
		return nullptr;
	}
	return constructor();
}

ConstructorFunc ClassDatabase::GetConstructor(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return nullptr;
	}

	ClassInfo const& classInfo = iter->second;
	return classInfo.m_creator;
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

std::vector<ClassInfo const*> ClassDatabase::GetRegisteredClasses(bool sortByInheritanceDepth)
{
	std::vector<ClassInfo const*> classes;
	classes.reserve(m_classInfoMap.size());
	for (auto const& classEntry : m_classInfoMap)
	{
		classes.push_back(&classEntry.second);
	}
	if (sortByInheritanceDepth)
	{
		size_t const maxDepth = classes.size();
		std::sort(
			classes.begin(),
			classes.end(),
			[maxDepth](ClassInfo const* left, ClassInfo const* right)
			{
				if (left == nullptr)
				{
					return false;
				}
				if (right == nullptr)
				{
					return true;
				}

				int const leftDepth  = GetClassInheritanceDepth(*left, maxDepth);
				int const rightDepth = GetClassInheritanceDepth(*right, maxDepth);
				if (leftDepth != rightDepth)
				{
					return leftDepth < rightDepth;
				}

				return left->m_className < right->m_className;
			});
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

	std::vector<PropertyInfo> properties;
	properties.reserve(iter->second.m_properties.size());
	for (std::unique_ptr<PropertyInfo> const& property : iter->second.m_properties)
	{
		if (property != nullptr)
		{
			properties.push_back(*property);
		}
	}

	return properties;
}

std::vector<PropertyInfo const*> ClassDatabase::GetAllProperties(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return {};
	}

	std::vector<PropertyInfo const*> properties = GetAllProperties(iter->second.m_parentClassName);
	for (std::unique_ptr<PropertyInfo> const& property : iter->second.m_properties)
	{
		if (property != nullptr)
		{
			properties.push_back(property.get());
		}
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

	for (std::unique_ptr<MethodInfo> const& method : classInfo->m_methods)
	{
		if (method != nullptr && method->m_name == methodName)
		{
			return method->m_bind.get();
		}
	}

	return nullptr;
}

void ClassDatabase::AddProperty(
	std::string const& className,
	PropertyInfo       propertyInfo,
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

	m_classInfoMap[className].m_properties.push_back(std::make_unique<PropertyInfo>(std::move(propertyInfo)));
}

std::vector<GlobalNamespaceInfo const*> ClassDatabase::GetRegisteredGlobalNamespaces()
{
	std::vector<GlobalNamespaceInfo const*> globalNamespaces;
	globalNamespaces.reserve(m_namespaceInfoMap.size());
	for (auto const& namespaceEntry : m_namespaceInfoMap)
	{
		globalNamespaces.push_back(&namespaceEntry.second);
	}
	return globalNamespaces;
}

MethodBind const* ClassDatabase::GetGlobalMethodBind(std::string const& namespaceName, std::string const& methodName)
{
	auto namespaceIter = m_namespaceInfoMap.find(namespaceName);
	if (namespaceIter == m_namespaceInfoMap.end())
	{
		return nullptr;
	}

	auto& methods = namespaceIter->second.m_methods;
	for (const auto& method : methods)
	{
		if (method && method->m_name == methodName)
		{
			return method->m_bind.get();
		}
	}

	return nullptr;
}

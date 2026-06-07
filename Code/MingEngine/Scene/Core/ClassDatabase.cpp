#include "MingEngine/Scene/Core/ClassDatabase.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"
#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

std::unordered_map<std::string, ClassDatabase::ClassInfo> ClassDatabase::m_classInfoMap;

void ClassDatabase::Startup()
{
	m_classInfoMap.clear();

	RegisterClass<Node>();
	RegisterClass<Node3D>();
	RegisterClass<Camera3D>();
	RegisterClass<DirectionalLight3D>();
	RegisterClass<PointLight3D>();
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
	return classInfo.m_creator();
}

ClassDatabase::ClassInfo const* ClassDatabase::GetClassInfo(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return nullptr;
	}

	return &iter->second;
}

std::vector<ClassDatabase::PropertyInfo> ClassDatabase::GetProperties(std::string const& className)
{
	auto iter = m_classInfoMap.find(className);
	if (iter == m_classInfoMap.end())
	{
		return {};
	}

	return iter->second.m_properties;
}

std::vector<ClassDatabase::PropertyInfo const*> ClassDatabase::GetAllProperties(std::string const& className)
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

ClassDatabase::PropertyInfo const* ClassDatabase::FindProperty(std::string const& className,
	std::string const&                                                            propertyName)
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

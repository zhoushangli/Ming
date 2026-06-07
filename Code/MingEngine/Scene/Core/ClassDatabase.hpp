#pragma once

#include "MingEngine/Scene/Core/Object.hpp"

#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class ClassDatabase
{
public:
	using PropertyValue = std::variant<bool, int, float, std::string, Vec3, EulerAngles, Matrix4x4>;
	enum class PropertyType
	{
		Bool,
		Int,
		Float,
		String,
		Vec3,
		EulerAngles,
		Matrix4x4,
	};

	struct PropertyInfo
	{
		PropertyInfo() = default;
		PropertyInfo(PropertyType type, std::string name) : m_type(type), m_name(name) {}

		PropertyType m_type;
		std::string  m_name;

		// The following getters and setters are expected to be set by AddProperty()
		// Because of the use of std::function, they can not directly point to member functions
		// but must be wrapped in lambdas that perform the appropriate casts
		std::function<PropertyValue(Object const&)>        m_getter;
		std::function<void(Object&, PropertyValue const&)> m_setter;
	};

	struct ClassInfo
	{
		std::string               m_className;
		std::string               m_parentClassName;
		std::function<Object*()>  m_creator;
		std::vector<PropertyInfo> m_properties;
	};

public:
	ClassDatabase()  = default;
	~ClassDatabase() = default;

	static void Startup();
	static void Shutdown();

	static Object*          CreateInstance(std::string const& className);
	static ClassInfo const* GetClassInfo(std::string const& className);

	template <typename T>
	static void RegisterClass()
	{
		std::string className = T::GetStaticClassName();
		ClassInfo   classInfo;
		classInfo.m_className       = className;
		classInfo.m_parentClassName = T::Super::GetStaticClassName();
		classInfo.m_creator         = &Creator<T>;
		m_classInfoMap[className]   = classInfo;
		T::InitializeClass();
	}

	template <typename T>
	static Object* Creator()
	{
		Object* object = new T();
		return object;
	}

	template <typename ClassType, typename ValueType>
	static void AddProperty(
		PropertyInfo property, ValueType (ClassType::*getter)() const, void (ClassType::*setter)(ValueType const&)
	)
	{
		property.m_getter = [getter](Object const& object) -> PropertyValue
		{
			ClassType const& typedObject = static_cast<ClassType const&>(object);
			return (typedObject.*getter)();
		};

		property.m_setter = [setter](Object& object, PropertyValue const& value)
		{
			ClassType& typedObject = static_cast<ClassType&>(object);
			(typedObject.*setter)(std::get<ValueType>(value));
		};

		m_classInfoMap[ClassType::GetStaticClassName()].m_properties.push_back(property);
	}
	static std::vector<PropertyInfo>        GetProperties(std::string const& className);
	static std::vector<PropertyInfo const*> GetAllProperties(std::string const& className);
	static PropertyInfo const*              FindProperty(std::string const& className, std::string const& propertyName);

private:
	static std::unordered_map<std::string, ClassInfo> m_classInfoMap;
};

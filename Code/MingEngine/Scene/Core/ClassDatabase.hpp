#pragma once

#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"
#include "MingEngine/Scene/Core/MethodBind.hpp"
#include "MingEngine/Scene/Core/Object.hpp"

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
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

	// Because of the use of unique_ptr
	// MethodInfo and ClassInfo must be moved, not copied
	// We indeed need to use std::unique_ptr here
	// Because we want method info deconstruct when the class info is destroyed
	struct MethodInfo
	{
		std::string                 m_name;
		std::unique_ptr<MethodBind> m_bind;
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
		std::string              m_className;
		std::string              m_parentClassName;
		std::function<Object*()> m_creator;
		bool                     m_canCreateInEditor = true;

		std::vector<PropertyInfo> m_properties;
		std::vector<MethodInfo>   m_methods;
	};

public:
	ClassDatabase()  = default;
	~ClassDatabase() = default;

	static void Startup();
	static void Shutdown();

	static Object*                       CreateInstance(std::string const& className);
	static ClassInfo const*              GetClassInfo(std::string const& className);
	static std::vector<ClassInfo const*> GetRegisteredClasses();
	static bool                          IsSubclassOf(std::string const& className, std::string const& baseClassName);

	template <typename T>
	static void RegisterClass(bool canCreateInEditor = true)
	{
		std::string className = T::GetStaticClassName();
		ClassInfo   classInfo;
		classInfo.m_className       = className;
		classInfo.m_parentClassName = T::Super::GetStaticClassName();
		if constexpr (!std::is_abstract_v<T>)
		{
			classInfo.m_creator = &Creator<T>;
		}
		classInfo.m_canCreateInEditor = canCreateInEditor;
		m_classInfoMap[className]     = std::move(classInfo);
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
		PropertyInfo property, ValueType (ClassType::*getter)() const, void (ClassType::*setter)(ValueType const&))
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

	template <typename ClassType, typename... Args>
	static void BindMethod(std::string const& methodName, void (ClassType::*method)(Args...))
	{
		MethodInfo methodInfo;
		methodInfo.m_name = methodName;
		methodInfo.m_bind = std::make_unique<VoidMethodBind<ClassType, Args...>>(method);
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename... Args>
	static void BindMethod(std::string const& methodName, void (ClassType::*method)(Args...) const)
	{
		MethodInfo methodInfo;
		methodInfo.m_name = methodName;
		methodInfo.m_bind = std::make_unique<ConstVoidMethodBind<ClassType, Args...>>(method);
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename ReturnType, typename... Args>
	static void BindMethod(std::string const& methodName, ReturnType (ClassType::*method)(Args...))
	{
		MethodInfo methodInfo;
		methodInfo.m_name = methodName;
		methodInfo.m_bind = std::make_unique<ReturnMethodBind<ClassType, ReturnType, Args...>>(method);
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename ReturnType, typename... Args>
	static void BindMethod(std::string const& methodName, ReturnType (ClassType::*method)(Args...) const)
	{
		MethodInfo methodInfo;
		methodInfo.m_name = methodName;
		methodInfo.m_bind = std::make_unique<ConstReturnMethodBind<ClassType, ReturnType, Args...>>(method);
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename... Args>

	static std::vector<PropertyInfo>        GetProperties(std::string const& className);
	static std::vector<PropertyInfo const*> GetAllProperties(std::string const& className);
	static PropertyInfo const*              FindProperty(std::string const& className, std::string const& propertyName);

private:
	static std::unordered_map<std::string, ClassInfo> m_classInfoMap;
};

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

class PackedScene;

class ClassDatabase
{
public:
	// Because of the use of unique_ptr
	// MethodInfo and ClassInfo must be moved, not copied
	// We indeed need to use std::unique_ptr here
	// Because we want method info deconstruct when the class info is destroyed
	struct MethodInfo
	{
		std::string m_name;
		std::unique_ptr<MethodBind> m_bind;
	};

	struct PropertyInfo
	{
		friend class ClassDatabase;
		friend class PackedScene;

	public:
		enum class PropertyUsageFlags : unsigned int
		{
			None    = 0,
			Storage = 1 << 0,
			Editor  = 1 << 1,
			Default = (1 << 0) | (1 << 1)
		};

		friend constexpr PropertyUsageFlags operator|(PropertyUsageFlags left, PropertyUsageFlags right)
		{
			return static_cast<PropertyUsageFlags>(
				static_cast<unsigned int>(left) | static_cast<unsigned int>(right));
		}

	public:
		PropertyInfo() = default;
		PropertyInfo(Variant::Type type, std::string name, PropertyUsageFlags usageFlags)
			: m_type(type), m_name(name), m_usageFlags(usageFlags)
		{
		}

		static constexpr bool HasFlag(PropertyUsageFlags value, PropertyUsageFlags flag)
		{
			return (static_cast<unsigned int>(value) & static_cast<unsigned int>(flag)) != 0;
		}
		bool HasUsage(PropertyUsageFlags usage) const { return HasFlag(m_usageFlags, usage); }
		MethodBind const* GetSetter() const { return m_setter; }
		MethodBind const* GetGetter() const { return m_getter; }

		Variant::Type     m_type       = Variant::Type::Empty;
		std::string       m_name;
		PropertyUsageFlags m_usageFlags = PropertyUsageFlags::None;

	private:
		std::string m_setterName;
		std::string m_getterName;
		MethodBind const* m_setter = nullptr;
		MethodBind const* m_getter = nullptr;
	};

	struct ClassInfo
	{
		std::string m_className;
		std::string m_parentClassName;
		std::function<Object*()> m_creator;
		bool m_canCreateInEditor = true;

		std::vector<PropertyInfo> m_properties;
		std::vector<MethodInfo> m_methods;
	};

public:
	ClassDatabase()  = default;
	~ClassDatabase() = default;

	static void Startup();
	static void Shutdown();

	static Object* CreateInstance(std::string const& className);
	static ClassInfo const* GetClassInfo(std::string const& className);
	static std::vector<ClassInfo const*> GetRegisteredClasses();
	static bool IsSubclassOf(std::string const& className, std::string const& baseClassName);

	static std::vector<PropertyInfo> GetProperties(std::string const& className);
	static std::vector<PropertyInfo const*> GetAllProperties(std::string const& className);
	static PropertyInfo const* FindProperty(std::string const& className, std::string const& propertyName);

	static MethodBind const* GetMethodBind(std::string const& className, std::string const& methodName);
	static void AddProperty(std::string const& className,
		PropertyInfo propertyInfo,
		std::string const& setterName,
		std::string const& getterName);

	template <typename T>
	static void RegisterClass(bool canCreateInEditor = true)
	{
		std::string className = T::GetStaticClassName();
		ClassInfo classInfo;
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

private:
	static std::unordered_map<std::string, ClassInfo> m_classInfoMap;
};

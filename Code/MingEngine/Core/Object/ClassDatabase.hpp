#pragma once

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Object/MethodBind.hpp"
#include "MingEngine/Core/Object/Object.hpp"

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

class PackedScene;

// Because of the use of unique_ptr
// MethodInfo and ClassInfo must be moved, not copied
// We indeed need to use std::unique_ptr here
// Because we want method info deconstruct when the class info is destroyed
struct MethodInfo
{
	std::string                 m_name;
	std::unique_ptr<MethodBind> m_bind;
	Variant::Type               m_returnType = Variant::Type::Empty;
	std::vector<Variant::Type>  m_argumentTypes;
	bool                        m_isConst = false;
};

struct PropertyInfo
{
	friend class ClassDatabase;
	friend class PackedScene;

public:
	// Hint indicates how we show the property in the editor
	// for example, ResourceType hint will show a resource picker in the editor
	enum class Hint
	{
		None,
		ResourceType,
	};

	// UsageFlags indicates how we use the property in the engine
	enum class UsageFlags : unsigned int
	{
		None      = 0,
		Storage   = 1 << 0,
		Inspector = 1 << 1,
		Group     = 1 << 2,
		Default   = Storage | Inspector
	};

	friend constexpr UsageFlags operator|(UsageFlags left, UsageFlags right)
	{
		return static_cast<UsageFlags>(static_cast<unsigned int>(left) | static_cast<unsigned int>(right));
	}

public:
	PropertyInfo() = default;
	PropertyInfo(Variant::Type type, std::string name, Hint hint, std::string hintData, UsageFlags usageFlags)
		: m_type(type), m_name(name), m_hint(hint), m_hintData(hintData), m_usageFlags(usageFlags)
	{
	}

	static constexpr bool HasFlag(UsageFlags value, UsageFlags flag)
	{
		return (static_cast<unsigned int>(value) & static_cast<unsigned int>(flag)) != 0;
	}
	bool              HasUsage(UsageFlags usage) const { return HasFlag(m_usageFlags, usage); }
	MethodBind const* GetSetter() const { return m_setter; }
	MethodBind const* GetGetter() const { return m_getter; }

public:
	Variant::Type m_type = Variant::Type::Empty;
	std::string   m_name;
	Hint          m_hint = Hint::None;
	std::string   m_hintData;
	UsageFlags    m_usageFlags = UsageFlags::None;

private:
	std::string       m_setterName;
	std::string       m_getterName;
	MethodBind const* m_setter = nullptr;
	MethodBind const* m_getter = nullptr;
};

struct ClassInfo
{
	std::string              m_className;
	std::string              m_parentClassName;
	std::function<Object*()> m_creator;
	bool                     m_canCreateInEditor = true;

	// We use unique_ptr to keep the memory stable for PropertyInfo and MethodInfo when vector resize
	// because our script system needs the method bind pointer to be stable to call them
	std::vector<std::unique_ptr<PropertyInfo>> m_properties;
	std::vector<std::unique_ptr<MethodInfo>>   m_methods;
};

class ClassDatabase
{
public:
	ClassDatabase()  = default;
	~ClassDatabase() = default;

	static void Startup();
	static void Shutdown();

	static Object*                       CreateInstance(std::string const& className);
	static ClassInfo const*              GetClassInfo(std::string const& className);
	static std::vector<ClassInfo const*> GetRegisteredClasses();
	static bool                          IsSubclassOf(std::string const& className, std::string const& baseClassName);

	static std::vector<PropertyInfo>        GetProperties(std::string const& className);
	static std::vector<PropertyInfo const*> GetAllProperties(std::string const& className);
	static PropertyInfo const*              FindProperty(std::string const& className, std::string const& propertyName);

	static MethodBind const* GetMethodBind(std::string const& className, std::string const& methodName);
	static void              AddProperty(
		std::string const& className,
		PropertyInfo       propertyInfo,
		std::string const& setterName,
		std::string const& getterName);

	// Only for Object class
	template <typename T>
	static void RegisterRootClass(bool canCreateInEditor = true)
	{
		std::string className = T::GetStaticClassName();
		ClassInfo   classInfo;
		classInfo.m_className = className;
		if constexpr (!std::is_abstract_v<T>)
		{
			classInfo.m_creator = &Creator<T>;
		}
		classInfo.m_canCreateInEditor = canCreateInEditor;
		m_classInfoMap[className]     = std::move(classInfo);
		T::InitializeClass();
	}

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

	template <typename ClassType, typename... Args>
	static void BindMethod(std::string const& methodName, void (ClassType::*method)(Args...))
	{
		std::unique_ptr<MethodInfo> methodInfo = std::make_unique<MethodInfo>();
		methodInfo->m_name                     = methodName;
		methodInfo->m_bind                     = std::make_unique<VoidMethodBind<ClassType, Args...>>(method);
		methodInfo->m_returnType               = Variant::Type::Empty;
		methodInfo->m_argumentTypes.reserve(sizeof...(Args));
		(methodInfo->m_argumentTypes.push_back(Variant::GetType<Args>()), ...);
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename... Args>
	static void BindMethod(std::string const& methodName, void (ClassType::*method)(Args...) const)
	{
		std::unique_ptr<MethodInfo> methodInfo = std::make_unique<MethodInfo>();
		methodInfo->m_name                     = methodName;
		methodInfo->m_bind                     = std::make_unique<ConstVoidMethodBind<ClassType, Args...>>(method);
		methodInfo->m_returnType               = Variant::Type::Empty;
		methodInfo->m_argumentTypes.reserve(sizeof...(Args));
		(methodInfo->m_argumentTypes.push_back(Variant::GetType<Args>()), ...);
		methodInfo->m_isConst = true;
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename ReturnType, typename... Args>
	static void BindMethod(std::string const& methodName, ReturnType (ClassType::*method)(Args...))
	{
		std::unique_ptr<MethodInfo> methodInfo = std::make_unique<MethodInfo>();
		methodInfo->m_name                     = methodName;
		methodInfo->m_bind       = std::make_unique<ReturnMethodBind<ClassType, ReturnType, Args...>>(method);
		methodInfo->m_returnType = Variant::GetType<ReturnType>();
		methodInfo->m_argumentTypes.reserve(sizeof...(Args));
		(methodInfo->m_argumentTypes.push_back(Variant::GetType<Args>()), ...);
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename ReturnType, typename... Args>
	static void BindMethod(std::string const& methodName, ReturnType (ClassType::*method)(Args...) const)
	{
		std::unique_ptr<MethodInfo> methodInfo = std::make_unique<MethodInfo>();
		methodInfo->m_name                     = methodName;
		methodInfo->m_bind       = std::make_unique<ConstReturnMethodBind<ClassType, ReturnType, Args...>>(method);
		methodInfo->m_returnType = Variant::GetType<ReturnType>();
		methodInfo->m_argumentTypes.reserve(sizeof...(Args));
		(methodInfo->m_argumentTypes.push_back(Variant::GetType<Args>()), ...);
		methodInfo->m_isConst = true;
		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

private:
	static std::unordered_map<std::string, ClassInfo> m_classInfoMap;
};

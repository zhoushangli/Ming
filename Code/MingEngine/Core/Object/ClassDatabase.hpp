#pragma once

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/Object/MethodBind.hpp"
#include "MingEngine/Core/Object/Object.hpp"

#include <functional>
#include <initializer_list>
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
struct ArgumentInfo
{
	Variant::Type m_type = Variant::Type::Empty;
	std::string   m_objectClassName; // "Node", "Resource", etc. Only used when m_type is Variant::Type::ObjectPtr
	// Parameter name used by the C# bindings, the return info keeps it empty.
	// e.g. BindMethod("SetName", &Node::SetName, {"name"}) stores "name" here
	std::string m_name;
};

struct MethodInfo
{
	std::string                 m_name;
	std::unique_ptr<MethodBind> m_bind;
	ArgumentInfo                m_returnInfo;
	std::vector<ArgumentInfo>   m_argumentInfos;
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
	std::string   m_setterName;
	std::string   m_getterName;

	// Set after AddProperty registers the class
	MethodBind const* m_setter = nullptr;
	MethodBind const* m_getter = nullptr;
};

enum class ApiType
{
	None,
	Runtime,
	Editor
};

using ConstructorFunc = Object* (*)();

struct ClassInfo
{
	std::string m_className;
	std::string m_parentClassName;
	ApiType     m_apiType = ApiType::None;
	// IsVirutal means this class can be instantiated
	// but it should use as the base class for other classes
	bool            m_isVirtual = false;
	ConstructorFunc m_creator   = nullptr;

	// Registration is the only writer and it runs before anyone keeps a pointer,
	// so MethodInfo may move while MethodBind stays reachable through MethodInfo::m_bind
	std::vector<PropertyInfo> m_properties;
	std::vector<MethodInfo>   m_methods;

	// Virtual methods are the override points exposed to C# scripts, they hold no MethodBind
	// so we generate the C# override from m_name and m_argumentInfos
	std::vector<MethodInfo> m_virtualMethods;
};

struct ConstantInfo
{
	std::string   m_name;
	Variant::Type m_type = Variant::Type::Empty;
	Variant       m_value;
};

struct GlobalNamespaceInfo
{
	std::string               m_namespaceName;
	std::vector<ConstantInfo> m_constants;
	std::vector<MethodInfo>   m_methods;
};

class ClassDatabase
{
public:
	ClassDatabase()  = default;
	~ClassDatabase() = default;

	static void Startup();
	static void Shutdown();

	static Object*                       CreateInstance(std::string const& className);
	static ConstructorFunc               GetConstructor(std::string const& className);
	static ClassInfo const*              GetClassInfo(std::string const& className);
	static std::vector<ClassInfo const*> GetRegisteredClasses(bool sortByInheritanceDepth = false);
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

	static void    SetApiType(ApiType apiType) { m_currentApiType = apiType; }
	static ApiType GetApiType() { return m_currentApiType; }

	// Only for Object class
	template <typename T>
	static void RegisterRootClass()
	{
		std::string className = T::GetStaticClassName();
		ClassInfo   classInfo;
		classInfo.m_className = className;
		classInfo.m_apiType   = ClassDatabase::m_currentApiType;
		classInfo.m_creator   = &Creator<T>;

		m_classInfoMap[className] = std::move(classInfo);
		T::InitializeClass();
	}

	// The meaning of virtual class is not equal to c++ virtual class
	// For class databse, it means it has a creator, but it should use as the base class for other classes
	// so normally we do not use the creator
	template <typename T>
	static void RegisterClass(bool isVirtual = false)
	{
		std::string className = T::GetStaticClassName();
		ClassInfo   classInfo;
		classInfo.m_className       = className;
		classInfo.m_parentClassName = T::Super::GetStaticClassName();
		classInfo.m_apiType         = ClassDatabase::m_currentApiType;
		classInfo.m_creator         = &Creator<T>;
		classInfo.m_isVirtual       = isVirtual;

		m_classInfoMap[className] = std::move(classInfo);
		T::InitializeClass();
	}

	// The meaning of abstract class is not equal to c++ abstract class
	// For class databse, it means we do not provide a creator for this class
	template <typename T>
	static void RegisterAbstractClass()
	{
		std::string className = T::GetStaticClassName();
		ClassInfo   classInfo;
		classInfo.m_className       = className;
		classInfo.m_parentClassName = T::Super::GetStaticClassName();
		classInfo.m_apiType         = ClassDatabase::m_currentApiType;

		m_classInfoMap[className] = std::move(classInfo);
		T::InitializeClass();
	}

	template <typename T>
	static void RegisterGlobalObject(T* object)
	{
		static_assert(std::is_base_of_v<Object, T>, "T must be derived from Object");
		m_globalObjectMap[object->GetClassName()] = object;
	}

	static Object* GetGlobalObject(std::string const& className)
	{
		auto iter = m_globalObjectMap.find(className);
		if (iter != m_globalObjectMap.end())
		{
			return iter->second;
		}
		return nullptr;
	}

	template <typename T>
	static Object* Creator()
	{
		static_assert(std::is_base_of_v<Object, T>, "ClassDatabase creators must construct Object subclasses.");
		Object* object = MemNew<T>();
		return object;
	}

	template <typename T>
	static ArgumentInfo GetArgumentInfo()
	{
		using CleanType   = std::remove_cv_t<std::remove_reference_t<T>>;
		using PointeeType = std::remove_cv_t<std::remove_pointer_t<CleanType>>;

		ArgumentInfo argumentInfo;
		argumentInfo.m_type = Variant::GetType<CleanType>();
		if constexpr (std::is_pointer_v<CleanType> && std::is_base_of_v<Object, PointeeType>)
		{
			argumentInfo.m_type            = Variant::Type::ObjectPtr;
			argumentInfo.m_objectClassName = PointeeType::GetStaticClassName();
		}
		return argumentInfo;
	}

	template <typename ClassType, typename ReturnType, typename... Args>
	static MethodBind* CreateMethodBind(ReturnType (ClassType::*method)(Args...))
	{
		return new MemberMethodBind<ClassType, ReturnType, Args...>(method);
	}

	template <typename ClassType, typename ReturnType, typename... Args>
	static MethodBind* CreateMethodBind(ReturnType (ClassType::*method)(Args...) const)
	{
		return new ConstMemberMethodBind<ClassType, ReturnType, Args...>(method);
	}

	template <typename ReturnType, typename... Args>
	static MethodBind* CreateMethodBind(ReturnType (*method)(Args...))
	{
		return new GlobalMethodBind<ReturnType, Args...>(method);
	}

	// The argument names are given in declaration order and every argument needs one.
	// e.g. BindMethod("SetName", &Node::SetName, {"name"})
	template <typename ClassType, typename ReturnType, typename... Args>
	static void BindMethod(
		std::string const& methodName,
		ReturnType (ClassType::*method)(Args...),
		std::initializer_list<std::string> const& argumentNames)
	{
		MethodInfo methodInfo;

		methodInfo.m_name       = methodName;
		methodInfo.m_bind       = std::unique_ptr<MethodBind>(CreateMethodBind(method));
		methodInfo.m_returnInfo = GetArgumentInfo<ReturnType>();
		methodInfo.m_argumentInfos.reserve(sizeof...(Args));
		(methodInfo.m_argumentInfos.push_back(GetArgumentInfo<Args>()), ...);
		ApplyArgumentNames(methodInfo, argumentNames, methodName);

		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	template <typename ClassType, typename ReturnType, typename... Args>
	static void BindMethod(
		std::string const& methodName,
		ReturnType (ClassType::*method)(Args...) const,
		std::initializer_list<std::string> const& argumentNames)
	{
		MethodInfo methodInfo;

		methodInfo.m_name       = methodName;
		methodInfo.m_bind       = std::unique_ptr<MethodBind>(CreateMethodBind(method));
		methodInfo.m_returnInfo = GetArgumentInfo<ReturnType>();
		methodInfo.m_argumentInfos.reserve(sizeof...(Args));
		(methodInfo.m_argumentInfos.push_back(GetArgumentInfo<Args>()), ...);
		methodInfo.m_isConst = true;
		ApplyArgumentNames(methodInfo, argumentNames, methodName);

		m_classInfoMap[ClassType::GetStaticClassName()].m_methods.push_back(std::move(methodInfo));
	}

	// Virtual methods are not callable through MethodBind, they only tell the generator
	// which C# override points a class exposes.
	// e.g. BindVirtualMethod("OnProcess", &Node::OnProcess, {"deltaSeconds"})
	template <typename ClassType, typename ReturnType, typename... Args>
	static void BindVirtualMethod(
		std::string const& methodName,
		[[maybe_unused]] ReturnType (ClassType::*method)(Args...),
		std::initializer_list<std::string> const& argumentNames)
	{
		MethodInfo methodInfo;

		methodInfo.m_name       = methodName;
		methodInfo.m_returnInfo = GetArgumentInfo<ReturnType>();
		methodInfo.m_argumentInfos.reserve(sizeof...(Args));
		(methodInfo.m_argumentInfos.push_back(GetArgumentInfo<Args>()), ...);
		ApplyArgumentNames(methodInfo, argumentNames, methodName);

		m_classInfoMap[ClassType::GetStaticClassName()].m_virtualMethods.push_back(std::move(methodInfo));
	}

	template <typename ReturnType, typename... Args>
	static void BindGlobalMethod(
		std::string const& namespaceName, std::string const& methodName, ReturnType (*method)(Args...))
	{
		MethodInfo methodInfo;

		methodInfo.m_name       = methodName;
		methodInfo.m_bind       = std::unique_ptr<MethodBind>(CreateMethodBind(method));
		methodInfo.m_returnInfo = GetArgumentInfo<ReturnType>();
		methodInfo.m_argumentInfos.reserve(sizeof...(Args));
		(methodInfo.m_argumentInfos.push_back(GetArgumentInfo<Args>()), ...);

		if (m_namespaceInfoMap.find(namespaceName) == m_namespaceInfoMap.end())
		{
			GlobalNamespaceInfo globalNamespaceInfo;
			globalNamespaceInfo.m_namespaceName = namespaceName;
			m_namespaceInfoMap[namespaceName]   = std::move(globalNamespaceInfo);
		}

		m_namespaceInfoMap[namespaceName].m_methods.push_back(std::move(methodInfo));
	}

	static std::vector<GlobalNamespaceInfo const*> GetRegisteredGlobalNamespaces();

	static MethodBind const* GetGlobalMethodBind(std::string const& namespaceName, std::string const& methodName);

	static void BindConstant(std::string const& namespaceName, std::string const& constantName, Variant value)
	{
		if (m_namespaceInfoMap.find(namespaceName) == m_namespaceInfoMap.end())
		{
			GlobalNamespaceInfo globalNamespaceInfo;
			globalNamespaceInfo.m_namespaceName = namespaceName;
			m_namespaceInfoMap[namespaceName]   = std::move(globalNamespaceInfo);
		}

		ConstantInfo constantInfo;
		constantInfo.m_name  = constantName;
		constantInfo.m_type  = value.GetType();
		constantInfo.m_value = value;

		m_namespaceInfoMap[namespaceName].m_constants.push_back(std::move(constantInfo));
	}

private:
	// Copy the given parameter names into the argument infos in declaration order.
	// e.g. ApplyArgumentNames(methodInfo, {"name"}, "SetName") names the single argument "name"
	static void ApplyArgumentNames(
		MethodInfo& methodInfo, std::vector<std::string> const& argumentNames, std::string const& methodName);

private:
	static std::unordered_map<std::string, ClassInfo>           m_classInfoMap;
	static std::unordered_map<std::string, GlobalNamespaceInfo> m_namespaceInfoMap;
	static std::unordered_map<std::string, Object*>             m_globalObjectMap;

	static ApiType m_currentApiType;
};

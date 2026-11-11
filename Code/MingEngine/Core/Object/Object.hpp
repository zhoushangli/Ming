#pragma once

#include "MingEngine/Core/Memory.hpp"
#include "MingEngine/Core/Object/ManagedGCHandle.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"

#include <memory>
#include <map>
#include <string>
#include <vector>

#define ADD_PROPERTY(propertyInfo, setterName, getterName)                                                             \
	ClassDatabase::AddProperty(GetStaticClassName(), propertyInfo, setterName, getterName)
#define ADD_GROUP(name) ClassDatabase::AddPropertyGroup(GetStaticClassName(), name)

#define BIND_ENUM(className, enumName)                                                                                 \
	ClassDatabase::BindEnumConstant<className>(#className, #enumName, className::enumName)

#define BIND_CONSTANT(namespaceName, constantName)                                                                     \
	ClassDatabase::BindConstant(#namespaceName, #constantName, static_cast<int>(namespaceName::constantName))

#define MCLASS(className, inheritName)                                                                                 \
public:                                                                                                                \
	using Self  = className;                                                                                           \
	using Super = inheritName;                                                                                         \
	static std::string GetStaticClassName()                                                                            \
	{                                                                                                                  \
		return #className;                                                                                             \
	}                                                                                                                  \
	virtual std::string GetClassName() const                                                                           \
	{                                                                                                                  \
		return #className;                                                                                             \
	}                                                                                                                  \
	static BindMethodsFunc GetBindMethodsFunc()                                                                        \
	{                                                                                                                  \
		return &Self::BindMethods;                                                                                     \
	}                                                                                                                  \
	static void InitializeClass()                                                                                      \
	{                                                                                                                  \
		static bool isClassInitialized = false;                                                                        \
		if (isClassInitialized)                                                                                        \
		{                                                                                                              \
			return;                                                                                                    \
		}                                                                                                              \
		Super::InitializeClass();                                                                                      \
		if (Self::GetBindMethodsFunc() != Super::GetBindMethodsFunc())                                                 \
		{                                                                                                              \
			Self::BindMethods();                                                                                       \
		}                                                                                                              \
		isClassInitialized = true;                                                                                     \
	}                                                                                                                  \
                                                                                                                       \
protected:                                                                                                             \
	void (Object::* GetOnNotificationFunc() const)(int)                                                                \
	{                                                                                                                  \
		return (void (Object::*)(int)) & Self::OnNotification;                                                         \
	}                                                                                                                  \
                                                                                                                       \
	virtual void NotificationForwardV(int notification)                                                                \
	{                                                                                                                  \
		Super::NotificationForwardV(notification);                                                                     \
                                                                                                                       \
		if (Self::GetOnNotificationFunc() != Super::GetOnNotificationFunc())                                           \
		{                                                                                                              \
			Self::OnNotification(notification);                                                                        \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	virtual void NotificationBackwardV(int notification)                                                               \
	{                                                                                                                  \
		if (Self::GetOnNotificationFunc() != Super::GetOnNotificationFunc())                                           \
		{                                                                                                              \
			Self::OnNotification(notification);                                                                        \
		}                                                                                                              \
                                                                                                                       \
		Super::NotificationBackwardV(notification);                                                                    \
	}

class ScriptInstance;
class Variant;
class Object;
class Callable;
struct CallError;

// These are for Memory.hpp override
// so we can call PostInitialize and PreDelete when creating and deleting objects
void PostInitializeHandler(Object* object);
bool PreDeleteHandler(Object* object);

class Object
{
	friend class ObjectDatabase;
	friend void PostInitializeHandler(Object* object);
	friend bool PreDeleteHandler(Object* object);

public:
	using BindMethodsFunc = void (*)();

public:
	enum
	{
		Notification_PostInitialize   = 0,
		Notification_PreDelete        = 1,
		Notification_PreDeleteCleanup = 2,
	};

private:
	struct Connection;

public:
	Object();
	virtual ~Object();

	Object(Object const&)            = delete;
	Object& operator=(Object const&) = delete;
	Object(Object&&)                 = delete;
	Object& operator=(Object&&)      = delete;

	// Class information and reflection
	// Will be overridden by the MCLASS macro in derived classes.
	static void            BindMethods();
	static BindMethodsFunc GetBindMethodsFunc();

	static std::string  GetStaticClassName();
	virtual std::string GetClassName() const;
	static void         InitializeClass();

	ObjectID GetObjectID() const;
	bool     IsPreDeleting() const { return m_isPreDeleting; }
	void     Notification(int notification, bool reverse = false);
	void     Free();

	bool HasMethod(std::string const& methodName) const;
	Variant Call(std::string const& methodName, std::vector<Variant> const& arguments = {});
	Variant Call(std::string const& methodName, std::vector<Variant> const& arguments, CallError& error);

	// Store synchronous connections on the emitter; signal names are dynamic in this minimal implementation.
	// e.g. button->Connect("pressed", Callable(inventory, "Open")) subscribes inventory to that button.
	bool Connect(std::string const& signalName, Callable const& callable);
	bool Disconnect(std::string const& signalName, Callable const& callable);
	bool IsConnected(std::string const& signalName, Callable const& callable) const;
	bool HasSignalConnections(std::string const& signalName) const;

	// Invoke a snapshot in connection order; connection changes affect subsequent emissions.
	// e.g. EmitSignal("pressed") calls each connected method before returning.
	bool EmitSignal(std::string const& signalName);
	bool EmitSignal(std::string const& signalName, std::vector<Variant> const& arguments);

	// Return the script resource attached to the current runtime instance.
	// e.g. GetScript() is empty while the instance is detached for reload.
	Variant GetScript() const;
	void    SetScript(Variant const& script);
	void    SetScriptInstance(ScriptInstance* scriptInstance);

	ScriptInstance* GetScriptInstance() const { return m_scriptInstance; }

	bool TrySetNativeBindingGCHandle(void* value);

	bool  IsNativeBindingGCHandleValid() const { return m_nativeBindingGCHandle.IsValid(); }
	void* GetNativeBindingGCHandle() const { return m_nativeBindingGCHandle.GetValue(); }

	void ReleaseNativeBindingGCHandle();
	void DetachNativeBindingGCHandle(void* expected);

protected:
	void OnNotification([[maybe_unused]] int notification) {}

	void NotificationForward(int notification);
	void NotificationBackward(int notification);

	virtual void NotificationForwardV([[maybe_unused]] int notification) {}

	virtual void NotificationBackwardV([[maybe_unused]] int notification) {}

	void (Object::* GetOnNotificationFunc() const)(int) { return &Object::OnNotification; }

private:
	void PostInitialize();
	bool PreDelete();
	void ClearSignalConnections();

protected:
	ObjectID        m_id            = ObjectID::Invalid;
	bool            m_isPreDeleting = false;
	ManagedGCHandle m_nativeBindingGCHandle;

	ScriptInstance* m_scriptInstance = nullptr;

private:
	std::map<std::string, std::vector<Connection>> m_signalMap;
	std::vector<Connection>                      m_signalSubscriptions;
};

class ObjectDatabase
{
public:
	static ObjectID AddInstance(Object* object);
	static void     RemoveInstance(Object* object);
	static Object*  GetInstance(ObjectID id);

	template <typename T>
	static T* GetInstance(ObjectID id)
	{
		Object* object = GetInstance(id);
		if (object == nullptr)
		{
			return nullptr;
		}

		return dynamic_cast<T*>(object);
	}

private:
	static std::vector<Object*> m_objectSlots;
	static uint32_t             m_nextObjectUID;
};

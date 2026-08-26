#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"

#include <memory>
#include <string>
#include <vector>

#define ADD_PROPERTY(propertyInfo, setterName, getterName)                                                             \
	ClassDatabase::AddProperty(GetStaticClassName(), propertyInfo, setterName, getterName)

#define BIND_ENUM(className, enumName)                                                                                 \
	ClassDatabase::BindConstant(#className, #enumName, static_cast<int>(className::enumName))

#define BIND_CONSTANT(namespaceName, constantName)                                                                     \
	ClassDatabase::BindConstant(#namespaceName, #constantName, static_cast<int>(namespaceName::constantName))

#define MCLASS(className, inheritName)                                                                                 \
public:                                                                                                                \
	using Self  = className;                                                                                           \
	using Super = inheritName;                                                                                         \
	static std::string     GetStaticClassName() { return #className; }                                                 \
	virtual std::string    GetClassName() const { return #className; }                                                 \
	static BindMethodsFunc GetBindMethodsFunc() { return &Self::BindMethods; }                                         \
	static void            InitializeClass()                                                                           \
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
	void (Object::* GetOnNotificationFunc() const)(int) { return (void (Object::*)(int)) & Self::OnNotification; }     \
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

class Object
{
	friend class ObjectDatabase;

public:
	using BindMethodsFunc = void (*)();

public:
	Object();
	virtual ~Object();

	Object(Object const&);
	Object& operator=(Object const&);
	Object(Object&&);
	Object& operator=(Object&&);

	// Class information and reflection
	// Will be overridden by the MCLASS macro in derived classes.
	static void            BindMethods();
	static BindMethodsFunc GetBindMethodsFunc();

	static std::string  GetStaticClassName();
	virtual std::string GetClassName() const;
	static void         InitializeClass();

	ObjectID GetObjectID() const;
	void     Notification(int notification, bool reverse = false);

protected:
	void OnNotification([[maybe_unused]] int notification) {}
	void NotificationForward(int notification);
	void NotificationBackward(int notification);

	virtual void NotificationForwardV([[maybe_unused]] int notification) {}
	virtual void NotificationBackwardV([[maybe_unused]] int notification) {}

	void (Object::* GetOnNotificationFunc() const)(int) { return &Object::OnNotification; }

private:
	ObjectID m_id = ObjectID::Invalid;
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

#pragma once

#include <memory>
#include <string>

#define ADD_PROPERTY(propertyName, setterName, getterName)                                                             \
	ClassDatabase::AddProperty(GetStaticClassName(), propertyName, setterName, getterName)

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

class ScriptInstance;

class Object
{
public:
	using BindMethodsFunc = void (*)();

public:
	Object() = default;
	virtual ~Object();

	// Class information and reflection
	// Will be overridden by the MCLASS macro in derived classes.
	static void            BindMethods();
	static BindMethodsFunc GetBindMethodsFunc();

	static std::string  GetStaticClassName();
	virtual std::string GetClassName() const;
	static void         InitializeClass();

	void Notification(int notification, bool reverse = false);

	void            SetScript(std::unique_ptr<ScriptInstance> scriptInstance);
	ScriptInstance* GetScript();

protected:
	void OnNotification([[maybe_unused]] int notification) {}
	void NotificationForward(int notification);
	void NotificationBackward(int notification);

	virtual void NotificationForwardV([[maybe_unused]] int notification) {}
	virtual void NotificationBackwardV([[maybe_unused]] int notification) {}

	void (Object::* GetOnNotificationFunc() const)(int) { return &Object::OnNotification; }

protected:
	std::unique_ptr<ScriptInstance> m_scriptInstance;
};

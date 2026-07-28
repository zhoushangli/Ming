#include "MingEngine/Core/Object/Object.hpp"

#include "MingEngine/Engine/Script/ScriptInstance.hpp"

Object::~Object() {}

Object::BindMethodsFunc Object::GetBindMethodsFunc() { return &Object::BindMethods; }

void Object::BindMethods() {}

std::string Object::GetStaticClassName() { return "Object"; }

std::string Object::GetClassName() const { return "Object"; }

void Object::InitializeClass()
{
	static bool isClassInitialized = false;
	if (isClassInitialized)
	{
		return;
	}

	isClassInitialized = true;
	BindMethods();
}

void Object::Notification(int notification, bool reverse)
{
	if (reverse)
	{
		NotificationBackward(notification);
	}
	else
	{
		NotificationForward(notification);
	}
}

void Object::NotificationForward(int notification)
{
	NotificationForwardV(notification);

	// if (m_scriptInstance != nullptr)
	// {
	// 	m_scriptInstance->Notification(notification, false);
	// }
}

void Object::NotificationBackward(int notification)
{
	// if (m_scriptInstance != nullptr)
	// {
	// 	m_scriptInstance->Notification(notification, true);
	// }

	NotificationBackwardV(notification);
}

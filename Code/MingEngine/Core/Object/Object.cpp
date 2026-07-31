#include "MingEngine/Core/Object/Object.hpp"

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
}

void Object::NotificationBackward(int notification)
{
	NotificationBackwardV(notification);
}

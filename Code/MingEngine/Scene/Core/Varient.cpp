#include "MingEngine/Scene/Core/Object.hpp"

Object::BindMethodsFunc Object::GetBindMethodsFunc() { return &Object::BindMethods; }

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

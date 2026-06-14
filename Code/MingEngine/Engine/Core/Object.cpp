#include "MingEngine/Scene/Core/Object.hpp"

#include "MingEngine/Engine/Script/ScriptInstance.hpp"

Object::~Object() {}

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

void Object::SetScript(std::unique_ptr<ScriptInstance> scriptInstance) { m_scriptInstance = std::move(scriptInstance); }

std::unique_ptr<ScriptInstance>& Object::GetScript() { return m_scriptInstance; }

#include "MingEngine/Engine/Script/CSharpScript.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"

bool CSharpScript::CopyFrom(Resource&& other)
{
	if (dynamic_cast<CSharpScript*>(&other) == nullptr)
	{
		return false;
	}

	MoveBaseFrom(std::move(other));
	return true;
}

bool CSharpScript::Instantiate(Object* owner)
{
	if (owner == nullptr)
	{
		return false;
	}

	// 1) Create a new CSharpScriptInstance
	CSharpScriptInstance* scriptInstance = new CSharpScriptInstance(Ref<CSharpScript>(this));
	scriptInstance->m_owner              = owner;
	owner->SetScriptInstance(std::unique_ptr<ScriptInstance>(scriptInstance));

	// 2) Initialize the script instance
	bool const created = g_engine->m_scriptSystem->CreateManagedScriptInstance(this, owner);

	if (!created || !scriptInstance->m_gcHandle.IsValid())
	{
		owner->SetScriptInstance(nullptr);
		return false;
	}

	return true;
}

CSharpScriptInstance::~CSharpScriptInstance()
{
	m_gcHandle.Release();

	m_script = nullptr;
	m_owner  = nullptr;
}

void CSharpScriptInstance::Notification(int notification, bool reverse) {}

bool CSharpScriptInstance::ReloadGCHandle(void* value)
{
	if (value == nullptr || m_gcHandle.IsValid())
	{
		return false;
	}

	m_gcHandle = ManagedGCHandle(value);
	return true;
}

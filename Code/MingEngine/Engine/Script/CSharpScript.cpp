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

	// 1) Create a new CSharpInstance
	CSharpInstance* scriptInstance = new CSharpInstance(Ref<CSharpScript>(this));
	scriptInstance->m_owner        = owner;
	owner->SetScriptInstance(scriptInstance);

	// 2) Initialize the script instance
	bool const created = g_engine->m_scriptSystem->CreateUserManagedInstance(this, owner);

	if (!created || !scriptInstance->m_gcHandle.IsValid())
	{
		owner->SetScriptInstance(nullptr);
		return false;
	}

	return true;
}

CSharpInstance::~CSharpInstance()
{
	if (m_gcHandle.IsValid())
	{
		m_gcHandle.Release();
	}

	m_script = nullptr;
	m_owner  = nullptr;
}

void CSharpInstance::Notification(int notification, [[maybe_unused]] bool reverse)
{
	switch (notification)
	{
	case Object::Notification_PreDeleteCleanup:
	{
		if (m_gcHandle.IsValid())
		{
			m_gcHandle.Release();
		}
	}
	}
}

bool CSharpInstance::ReloadGCHandle(void* value)
{
	if (value == nullptr)
	{
		return false;
	}

	if (m_gcHandle.IsValid())
	{
		m_gcHandle.Release();
	}
	m_gcHandle = ManagedGCHandle(value);
	return true;
}

bool CSharpInstance::ValidateAfterGC() const
{
	if (!m_gcHandle.IsValid() || m_owner == nullptr)
	{
		return false;
	}

	return g_engine->m_scriptSystem->ValidateManagedScriptInstance(m_gcHandle.GetValue(), m_owner);
}

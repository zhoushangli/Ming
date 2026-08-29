#pragma once

#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptInstance.hpp"
#include "MingEngine/Engine/Script/ManagedGCHandle.hpp"

class CSharpScript final : public Script
{
	MCLASS(CSharpScript, Script)

public:
	bool CopyFrom(Resource&& other) override;

	bool Instantiate(Object* owner) override;
};

class CSharpInstance : public ScriptInstance
{
	friend class CSharpScript;

public:
	CSharpInstance(Ref<CSharpScript> script) : m_script(script) {}
	~CSharpInstance() override;

	Object*     GetOwner() const override { return m_owner; }
	Ref<Script> GetScript() const override { return m_script; }

	void Notification(int notification, bool reverse) override;

	bool ReloadGCHandle(void* value) override;

	bool ValidateAfterGC() const;

private:
	Ref<CSharpScript> m_script;
	Object*           m_owner = nullptr;
	ManagedGCHandle   m_gcHandle;
};
#pragma once

#include "MingEngine/Core/Object/ManagedGCHandle.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptInstance.hpp"

class CSharpScript : public Script
{
	MCLASS(CSharpScript, Script)

public:
	~CSharpScript() override;
	bool CopyFrom(Resource&& other) override;

	bool Instantiate(Object* owner) override;
	bool InstantiatePlaceHolder(Object* owner) override;
};

class CSharpInstance : public ScriptInstance
{
	friend class CSharpScript;

public:
	CSharpInstance(Ref<CSharpScript> script) : m_script(script) {}

	~CSharpInstance() override;

	Object*     GetOwner() const override { return m_owner; }
	Ref<Script> GetScript() const override { return m_script; }

	using ScriptInstance::Call;

	bool HasMethod(std::string const& methodName) const override;
	Variant Call(std::string const& methodName, std::vector<Variant> const& args, CallError& error) override;
	void Notification(int notification, bool reverse) override;

	void* GetGCHandle() const { return m_gcHandle.GetValue(); }

	bool ReloadGCHandle(void* value) override;

private:
	Ref<CSharpScript> m_script;
	Object*           m_owner = nullptr;
	ManagedGCHandle   m_gcHandle;
};

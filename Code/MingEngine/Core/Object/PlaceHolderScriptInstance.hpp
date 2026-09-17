#pragma once

#include "MingEngine/Core/Object/ScriptInstance.hpp"

class PlaceHolderScriptInstance : public ScriptInstance
{
public:
	PlaceHolderScriptInstance();
	PlaceHolderScriptInstance(Ref<Script> script, Object* owner) : m_owner(owner), m_script(script) {}
	~PlaceHolderScriptInstance();

	// Inherit from ScriptInstance
	Object*     GetOwner() const override { return m_owner; }
	Ref<Script> GetScript() const override { return m_script; }
	bool        ReloadGCHandle([[maybe_unused]] void* value) override { return false; }

	bool Call(
		[[maybe_unused]] std::string const&          methodName,
		[[maybe_unused]] std::vector<Variant> const& args,
		[[maybe_unused]] Variant&                    ret) override
	{
		return false;
	}
	void Notification([[maybe_unused]] int notification, [[maybe_unused]] bool reverse) override { /* Do nothing */ };

private:
	Object*     m_owner = nullptr;
	Ref<Script> m_script;
};
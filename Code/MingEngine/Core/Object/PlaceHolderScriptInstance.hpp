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
	bool        ReloadGCHandle(void* value) override {return false; }

	void Notification(int notification, bool reverse) override { /* Do nothing */ };

private:
	Object*     m_owner = nullptr;
	Ref<Script> m_script;
};
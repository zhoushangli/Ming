#pragma once

#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"

#include <memory>

class asIScriptObject;
class asIScriptModule;
class asIScriptFunction;
class asIScriptContext;

class ScriptInstance
{
	friend class ScriptSystem; // Only the ScriptSystem can create new ScriptInstance objects

public:
	~ScriptInstance();

	void Notification(int notification, bool reverse = false);

	bool CallReady();
	bool CallProcess(float deltaSeconds);
	bool CallEnterTree();
	bool CallExitTree();

	Object* GetOwner() const { return m_owner; }
	Ref<Script> GetScript() const { return m_script; }

private:
	ScriptInstance() = default;

	ScriptInstance(ScriptInstance const&)            = delete;
	ScriptInstance& operator=(ScriptInstance const&) = delete;

	void Destroy();

	bool Execute(asIScriptFunction* function);
	bool Execute(asIScriptFunction* function, float deltaSeconds);

private:
	Object* m_owner = nullptr;

	asIScriptModule* m_module = nullptr;
	asIScriptObject* m_object = nullptr;

	asIScriptFunction* m_enterTreeFunction = nullptr;
	asIScriptFunction* m_exitTreeFunction  = nullptr;
	asIScriptFunction* m_readyFunction     = nullptr;
	asIScriptFunction* m_processFunction   = nullptr;

	Ref<Script> m_script;
};

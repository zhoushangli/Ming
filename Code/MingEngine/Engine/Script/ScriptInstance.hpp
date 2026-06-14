#pragma once

#include "MingEngine/Engine/Script/ScriptModule.hpp"
#include "MingEngine/Scene/Core/Object.hpp"

#include <memory>

class asIScriptObject;
class asIScriptFunction;
class asIScriptContext;

class ScriptInstance
{
	friend class ScriptSystem; // Only the ScriptSystem can create new ScriptInstance objects

public:
	~ScriptInstance();

	bool CallReady();
	bool CallProcess(float deltaSeconds);

private:
	ScriptInstance() = default;

	ScriptInstance(ScriptInstance const&)            = delete;
	ScriptInstance& operator=(ScriptInstance const&) = delete;

	static std::unique_ptr<ScriptInstance> Create(ScriptModule& module, Object& owner);

	void Destroy();

	bool Execute(asIScriptFunction* function);
	bool Execute(asIScriptFunction* function, float deltaSeconds);

	Object* GetOwner() const;

private:
	Object* m_owner;

	ScriptModule*    m_module = nullptr;
	asIScriptObject* m_object = nullptr;

	asIScriptFunction* m_readyFunction   = nullptr;
	asIScriptFunction* m_processFunction = nullptr;
};
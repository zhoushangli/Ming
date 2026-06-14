#include "MingEngine/Engine/Script/ScriptInstance.hpp"

#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

ScriptInstance::~ScriptInstance() { Destroy(); }

std::unique_ptr<ScriptInstance> ScriptInstance::Create(ScriptModule& module, Object& owner)
{
	std::unique_ptr<ScriptInstance> instance(new ScriptInstance());
	
	instance->m_owner = &owner;

	asIScriptModule* scriptModule = module.GetScriptModule();
	asITypeInfo*     scriptType   = module.GetScriptType();

	if (scriptModule == nullptr || scriptType == nullptr)
	{
		DebuggerPrintf("Error: Invalid script module or script type.\n");
		return nullptr;
	}

	asIScriptEngine* engine = scriptModule->GetEngine();
	if (engine == nullptr)
	{
		DebuggerPrintf("Error: Script module does not have a valid script engine.\n");
		return nullptr;
	}

	void* object = engine->CreateScriptObject(scriptType);
	if (object == nullptr)
	{
		DebuggerPrintf("Error: Failed to create script object.\n");
		return nullptr;
	}

	instance->m_module = &module;
	instance->m_object = static_cast<asIScriptObject*>(object);

	instance->m_readyFunction   = scriptType->GetMethodByDecl("void _Ready()");
	instance->m_processFunction = scriptType->GetMethodByDecl("void _Process(float)");

	return instance;
}

void ScriptInstance::Destroy()
{
	if (m_object != nullptr)
	{
		m_object->Release();
		m_object = nullptr;
	}

	m_readyFunction   = nullptr;
	m_processFunction = nullptr;
	m_module          = nullptr;
}

bool ScriptInstance::CallReady() { return Execute(m_readyFunction); }

bool ScriptInstance::CallProcess(float deltaSeconds) { return Execute(m_processFunction, deltaSeconds); }

bool ScriptInstance::Execute(asIScriptFunction* function)
{
	if (function == nullptr)
	{
		return true;
	}

	if (m_object == nullptr || m_module == nullptr)
	{
		DebuggerPrintf(
			"Error: Cannot execute script function because the script instance is not properly initialized.\n");
		return false;
	}

	asIScriptEngine*  engine  = m_module->GetScriptModule()->GetEngine();
	asIScriptContext* context = engine->CreateContext();

	if (context == nullptr)
	{
		DebuggerPrintf("Error: Failed to create script context.\n");
		return false;
	}

	int result = context->Prepare(function);
	if (result >= 0)
	{
		result = context->SetObject(m_object);
	}

	if (result >= 0)
	{
		result = context->Execute();
	}

	if (result == asEXECUTION_EXCEPTION)
	{
		DebuggerPrintf("Script exception: %s\n", context->GetExceptionString());
	}

	context->Release();

	return result == asEXECUTION_FINISHED;
}

bool ScriptInstance::Execute(asIScriptFunction* function, float deltaSeconds)
{
	if (function == nullptr)
	{
		return true;
	}

	if (m_object == nullptr || m_module == nullptr)
	{
		return false;
	}

	asIScriptEngine* engine = m_module->GetScriptModule()->GetEngine();

	asIScriptContext* context = engine->CreateContext();
	if (context == nullptr)
	{
		return false;
	}

	int result = context->Prepare(function);

	if (result >= 0)
		result = context->SetObject(m_object);

	if (result >= 0)
		result = context->SetArgFloat(0, deltaSeconds);

	if (result >= 0)
		result = context->Execute();

	if (result == asEXECUTION_EXCEPTION)
	{
		DebuggerPrintf("Script exception: %s\n", context->GetExceptionString());
	}

	context->Release();

	return result == asEXECUTION_FINISHED;
}

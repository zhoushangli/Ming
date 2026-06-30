#include "MingEngine/Engine/Script/ScriptInstance.hpp"

#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

ScriptInstance::~ScriptInstance() { Destroy(); }

void ScriptInstance::Notification(int notification, bool reverse)
{
	notification;
	reverse;
}

void ScriptInstance::Destroy()
{
	if (m_object != nullptr)
	{
		m_object->Release();
		m_object = nullptr;
	}

	m_enterTreeFunction = nullptr;
	m_exitTreeFunction  = nullptr;
	m_readyFunction     = nullptr;
	m_processFunction   = nullptr;
	m_owner             = nullptr;
}

bool ScriptInstance::CallReady() { return Execute(m_readyFunction); }

bool ScriptInstance::CallProcess(float deltaSeconds) { return Execute(m_processFunction, deltaSeconds); }

bool ScriptInstance::CallEnterTree() { return Execute(m_enterTreeFunction); }

bool ScriptInstance::CallExitTree() { return Execute(m_exitTreeFunction); }

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

	asIScriptEngine*  engine  = m_module->GetEngine();
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

	asIScriptEngine* engine = m_module->GetEngine();

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

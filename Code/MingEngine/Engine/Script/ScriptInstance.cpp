#include "MingEngine/Engine/Script/ScriptInstance.hpp"

#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

ScriptInstance::~ScriptInstance() { Destroy(); }

void ScriptInstance::Notification(int notification, bool reverse)
{
	notification;
	reverse;
	// switch (notification)
	// {
	// case (int)Node::NotificationType::EnterTree:
	// 	CallEnterTree();
	// 	break;
	// case (int)Node::NotificationType::ExitTree:
	// 	CallExitTree();
	// 	break;
	// case (int)Node::NotificationType::Ready:
	// 	CallReady();
	// 	break;
	// case (int)Node::NotificationType::Process:
	// 	CallProcess(0.0f);
	// 	break;
	// }
}

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

	asUINT propertyCount = instance->m_object->GetPropertyCount();
	for (asUINT propertyIndex = 0; propertyIndex < propertyCount; ++propertyIndex)
	{
		char const* propertyName = instance->m_object->GetPropertyName(propertyIndex);
		if (propertyName != nullptr && strcmp(propertyName, "_native") == 0)
		{
			void* ownerPropertyAddress = instance->m_object->GetAddressOfProperty(propertyIndex);
			*static_cast<Object**>(ownerPropertyAddress) = &owner;
			break;
		}
	}

	instance->m_enterTreeFunction = scriptType->GetMethodByDecl("void _EnterTree()");
	instance->m_exitTreeFunction  = scriptType->GetMethodByDecl("void _ExitTree()");
	instance->m_readyFunction     = scriptType->GetMethodByDecl("void _Ready()");
	instance->m_processFunction   = scriptType->GetMethodByDecl("void _Process(float)");

	return instance;
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
	m_module            = nullptr;
	m_owner             = nullptr;
}

bool ScriptInstance::CallReady() { return Execute(m_readyFunction); }

bool ScriptInstance::CallProcess(float deltaSeconds) { return Execute(m_processFunction, deltaSeconds); }

bool ScriptInstance::CallEnterTree() { return Execute(m_enterTreeFunction); }

bool ScriptInstance::CallExitTree() { return Execute(m_exitTreeFunction); }

Object* ScriptInstance::GetOwner() const { return m_owner; }

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

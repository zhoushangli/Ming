#pragma once

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "ThirdParty/DotNetHost/coreclr_delegates.h"

#include <utility>

//---------------------------------------------------------------------------

// Native callbacks

using LogUtf8Func            = int32_t(CORECLR_DELEGATE_CALLTYPE*)(uint8_t const* text, int32_t length);
using CreateObjectFunc       = void*(CORECLR_DELEGATE_CALLTYPE*)(char const* className);
using GetObjectClassNameFunc = char const*(CORECLR_DELEGATE_CALLTYPE*)(void* objectPtr);
using GetMethodBindFunc      = void const*(CORECLR_DELEGATE_CALLTYPE*)(uint8_t const* className,
																	   int32_t        classNameLength,
																	   uint8_t const* methodName,
																	   int32_t        methodNameLength);
using MethodBindPtrCallFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(void const* methodBind, void* objectPtr, void** args, void* retPtr);
using CreateStringFunc              = void*(CORECLR_DELEGATE_CALLTYPE*)(void const* str, int32_t length);
using GetStringBufferFunc           = void*(CORECLR_DELEGATE_CALLTYPE*)(void* str);
using GetStringLengthFunc           = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void const* str);
using DestroyStringFunc             = void(CORECLR_DELEGATE_CALLTYPE*)(void const* str);
using BindManagedScriptInstanceFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* owner, void* gcHandle);

struct NativeCallbacks
{
	LogUtf8Func                   m_logUtf8                   = nullptr;
	CreateObjectFunc              m_createObject              = nullptr;
	GetObjectClassNameFunc        m_getObjectClassName        = nullptr;
	GetMethodBindFunc             m_getMethodBind             = nullptr;
	MethodBindPtrCallFunc         m_methodBindPtrCall         = nullptr;
	CreateStringFunc              m_createString              = nullptr;
	GetStringBufferFunc           m_getStringBuffer           = nullptr;
	GetStringLengthFunc           m_getStringLength           = nullptr;
	DestroyStringFunc             m_destroyString             = nullptr;
	BindManagedScriptInstanceFunc m_bindManagedScriptInstance = nullptr;
};

//---------------------------------------------------------------------------

// Managed callbacks

using ManagedPingFunc        = int32_t(CORECLR_DELEGATE_CALLTYPE*)();
using CreateTestGCHandleFunc = void*(CORECLR_DELEGATE_CALLTYPE*)();
using InvokeTestGCHandleFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle);
using FreeGCHandleFunc       = void(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle);
using CollectAndGetStateFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(int32_t* allocated, int32_t* freed, int32_t* targetAlive);
using CreateManagedScriptInstanceFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* script, void* owner);

struct ManagedCallbacks
{
	ManagedPingFunc                 m_ping                        = nullptr;
	CreateTestGCHandleFunc          m_createTestGCHandle          = nullptr;
	InvokeTestGCHandleFunc          m_invokeTestGCHandle          = nullptr;
	FreeGCHandleFunc                m_freeGCHandle                = nullptr;
	CollectAndGetStateFunc          m_collectAndGetState          = nullptr;
	CreateManagedScriptInstanceFunc m_createManagedScriptInstance = nullptr;
};

//---------------------------------------------------------------------------

using InitializeFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(
	NativeCallbacks const* nativeCallbacks,
	int32_t                nativeCallbacksSize,
	ManagedCallbacks*      managedCallbacks,
	int32_t                managedCallbacksSize);

struct ScriptSystemConfig
{
	bool m_isEnabled = true;
};

class Script;

class ScriptSystem : public SystemBase
{
	MCLASS(ScriptSystem, SystemBase)

public:
	ScriptSystem(ScriptSystemConfig const& config);

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	void FreeGCHandle(void* gcHandle);
	bool CreateManagedScriptInstance(Script* script, Object* owner);

private:
	using ShutdownFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)();

	bool InitializeDotNetRuntime();

	void*        m_hostfxrModule = nullptr;
	ShutdownFunc m_shutdown      = nullptr;
	bool         m_isInitialized = false;

	ManagedCallbacks m_managedCallbacks = {};
};

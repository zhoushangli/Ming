#pragma once

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "ThirdParty/DotNetHost/coreclr_delegates.h"

#include <utility>

//---------------------------------------------------------------------------

// Native callbacks

using LogUtf8Func            = int32_t(CORECLR_DELEGATE_CALLTYPE*)(uint8_t const* text, int32_t length);
using GetObjectClassNameFunc = char const*(CORECLR_DELEGATE_CALLTYPE*)(void* objectPtr);
using GetMethodBindFunc      = void const*(CORECLR_DELEGATE_CALLTYPE*)(uint8_t const* className,
																	   int32_t        classNameLength,
																	   uint8_t const* methodName,
																	   int32_t        methodNameLength);
using MethodBindPtrCallFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(void const* methodBind, void* objectPtr, void** args, void* retPtr);
using CreateStringFunc                          = void*(CORECLR_DELEGATE_CALLTYPE*)(void const* str, int32_t length);
using GetStringBufferFunc                       = void*(CORECLR_DELEGATE_CALLTYPE*)(void* str);
using GetStringLengthFunc                       = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void const* str);
using DestroyStringFunc                         = void(CORECLR_DELEGATE_CALLTYPE*)(void const* str);
using TieNativeManagedToUnmanagedFunc           = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle, void* native);
using GetConstructorFunc                        = ConstructorFunc(CORECLR_DELEGATE_CALLTYPE*)(void* name);
using UnmanagedGetInstanceBindingManagedFunc    = void*(CORECLR_DELEGATE_CALLTYPE*)(void* native);
using UnmanagedInstanceBindingCreateManagedFunc = void*(CORECLR_DELEGATE_CALLTYPE*)(void* native);

struct NativeCallbacks
{
	LogUtf8Func                               m_logUtf8                               = nullptr;
	GetObjectClassNameFunc                    m_getObjectClassName                    = nullptr;
	GetMethodBindFunc                         m_getMethodBind                         = nullptr;
	MethodBindPtrCallFunc                     m_methodBindPtrCall                     = nullptr;
	CreateStringFunc                          m_createString                          = nullptr;
	GetStringBufferFunc                       m_getStringBuffer                       = nullptr;
	GetStringLengthFunc                       m_getStringLength                       = nullptr;
	DestroyStringFunc                         m_destroyString                         = nullptr;
	TieNativeManagedToUnmanagedFunc           m_tieNativeManagedToUnmanaged           = nullptr;
	GetConstructorFunc                        m_getConstructor                        = nullptr;
	UnmanagedGetInstanceBindingManagedFunc    m_unmanagedGetInstanceBindingManaged    = nullptr;
	UnmanagedInstanceBindingCreateManagedFunc m_unmanagedInstanceBindingCreateManaged = nullptr;
};

//---------------------------------------------------------------------------

// Managed callbacks

using ManagedPingFunc        = int32_t(CORECLR_DELEGATE_CALLTYPE*)();
using CreateTestGCHandleFunc = void*(CORECLR_DELEGATE_CALLTYPE*)();
using InvokeTestGCHandleFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle);
using ReleaseGCHandleFunc    = void(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle);
using CollectAndGetStateFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(int32_t* allocated, int32_t* freed, int32_t* targetAlive);
using CreateUserManagedInstanceFunc     = void*(CORECLR_DELEGATE_CALLTYPE*)(void* script, void* owner);
using CreateNativeManagedInstanceFunc   = void*(CORECLR_DELEGATE_CALLTYPE*)(void* nativeClassName, void* owner);
using ValidateManagedScriptInstanceFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle, void* expectedOwner);
using CollectAndGetManagedScriptStateFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(int32_t* allocated, int32_t* disposed, int32_t* freed, int32_t* targetAlive);
using CreateNativeManagedWrapperForSmokeFunc = void*(CORECLR_DELEGATE_CALLTYPE*)();
using ValidateNativeManagedWrapperFunc       = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* owner);
using CollectAndGetNativeBindingStateFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(int32_t* allocated, int32_t* disposed, int32_t* freed);

struct ManagedCallbacks
{
	ManagedPingFunc                        m_ping                               = nullptr;
	CreateTestGCHandleFunc                 m_createTestGCHandle                 = nullptr;
	InvokeTestGCHandleFunc                 m_invokeTestGCHandle                 = nullptr;
	ReleaseGCHandleFunc                    m_releaseGCHandle                    = nullptr;
	CollectAndGetStateFunc                 m_collectAndGetState                 = nullptr;
	CreateUserManagedInstanceFunc          m_createUserManagedInstance          = nullptr;
	CreateNativeManagedInstanceFunc        m_createNativeManagedInstance        = nullptr;
	ValidateManagedScriptInstanceFunc      m_validateManagedScriptInstance      = nullptr;
	CollectAndGetManagedScriptStateFunc    m_collectAndGetManagedScriptState    = nullptr;
	CreateNativeManagedWrapperForSmokeFunc m_createNativeManagedWrapperForSmoke = nullptr;
	ValidateNativeManagedWrapperFunc       m_validateNativeManagedWrapper       = nullptr;
	CollectAndGetNativeBindingStateFunc    m_collectAndGetNativeBindingState    = nullptr;
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
class CSharpScript;
struct MingString;

class ScriptSystem : public SystemBase
{
	MCLASS(ScriptSystem, SystemBase)

public:
	ScriptSystem(ScriptSystemConfig const& config);

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	void ReleaseGCHandle(void* gcHandle);
	bool CreateUserManagedInstance(CSharpScript* script, Object* owner);

	bool ValidateManagedScriptInstance(void* gcHandle, Object* expectedOwner)
	{
		if (gcHandle == nullptr || expectedOwner == nullptr
			|| m_managedCallbacks.m_validateManagedScriptInstance == nullptr)
		{
			return false;
		}

		return m_managedCallbacks.m_validateManagedScriptInstance(gcHandle, expectedOwner) != 0;
	}

	void CollectAndGetManagedScriptState(int32_t& allocated, int32_t& disposed, int32_t& freed, int32_t& targetAlive);

	void* GetOrCreateNativeManagedWrapper(Object* owner);

private:
	using ShutdownFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)();

	bool InitializeDotNetRuntime();
	void RunNativeBindingSmoke();

	void*        m_hostfxrModule = nullptr;
	ShutdownFunc m_shutdown      = nullptr;
	bool         m_isInitialized = false;

	ManagedCallbacks m_managedCallbacks = {};
};

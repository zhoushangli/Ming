#pragma once

#include "ThirdParty/DotNetHost/coreclr_delegates.h"

using LogUtf8Func            = int32_t(CORECLR_DELEGATE_CALLTYPE*)(uint8_t const* text, int32_t length);
using CreateObjectFunc       = void*(CORECLR_DELEGATE_CALLTYPE*)(char const* className);
using GetObjectClassNameFunc = char const*(CORECLR_DELEGATE_CALLTYPE*)(void* objectPtr);
using GetMethodBindFunc      = void const*(CORECLR_DELEGATE_CALLTYPE*)(uint8_t const* className,
																	   int32_t        classNameLength,
																	   uint8_t const* methodName,
																	   int32_t        methodNameLength);
using MethodBindPtrCallFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(void const* methodBind, void* objectPtr, void** args, void* retPtr);

struct NativeCallbacks
{
	LogUtf8Func            m_logUtf8            = nullptr;
	CreateObjectFunc       m_createObject       = nullptr;
	GetObjectClassNameFunc m_getObjectClassName = nullptr;
	GetMethodBindFunc      m_getMethodBind      = nullptr;
	MethodBindPtrCallFunc  m_methodBindPtrCall  = nullptr;
};

using InitializeFunc =
	int32_t(CORECLR_DELEGATE_CALLTYPE*)(NativeCallbacks const* nativeCallbacks, int32_t nativeCallbacksSize);
using ShutdownFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)();

class DotNetHost
{
public:
	bool Initialize();
	void Shutdown();

private:
	void*          m_hostfxrModule = nullptr;
	InitializeFunc m_initialize    = nullptr;
	ShutdownFunc   m_shutdown      = nullptr;
	bool           m_isInitialized = false;
};
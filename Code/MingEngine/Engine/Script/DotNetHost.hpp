#pragma once

#include "ThirdParty/DotNetHost/coreclr_delegates.h"

using LogUtf8Func = int32_t(CORECLR_DELEGATE_CALLTYPE *)(uint8_t const *text, int32_t length);

struct NativeCallbacks
{
	LogUtf8Func m_logUtf8 = nullptr;
};

using InitializeFunc = int32_t(CORECLR_DELEGATE_CALLTYPE *)(NativeCallbacks const *nativeCallbacks, int32_t nativeCallbacksSize);
using ShutdownFunc = int32_t(CORECLR_DELEGATE_CALLTYPE *)();

class DotNetHost
{
public:
	bool Initialize();
	void Shutdown();

private:
	void *m_hostfxrModule = nullptr;
	InitializeFunc m_initialize = nullptr;
	ShutdownFunc m_shutdown = nullptr;
	bool m_isInitialized = false;
};
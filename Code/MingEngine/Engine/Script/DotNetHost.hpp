#pragma once

#include "ThirdParty/DotNetHost/coreclr_delegates.h"

using ManagedEntryPoint = void(CORECLR_DELEGATE_CALLTYPE *)();

class DotNetHost
{
public:
	bool Initialize();
	void Shutdown();

private:
	void *m_hostfxrModule = nullptr;
	ManagedEntryPoint m_initialize = nullptr;
	ManagedEntryPoint m_shutdown = nullptr;
	bool m_isInitialized = false;
};
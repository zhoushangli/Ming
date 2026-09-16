#pragma once

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/String.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Script/CSharpScript.hpp"

#include "ThirdParty/DotNetHost/coreclr_delegates.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

//---------------------------------------------------------------------------

// Native callbacks

using LogFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(String const* text);

// Copy borrowed UTF-32 code points into a String owned by the managed caller.
// e.g. CreateString(codes, 4, outString) leaves outString owning a 5 element buffer.
using CreateStringFunc = void(CORECLR_DELEGATE_CALLTYPE*)(char32_t const* str, int32_t length, String* outString);

// Release the buffer owned by the given managed side string.
// e.g. DestroyString(&text) empties text after freeing its UTF-32 buffer.
using DestroyStringFunc = void(CORECLR_DELEGATE_CALLTYPE*)(String* str);
using GetMethodBindFunc = void const*(CORECLR_DELEGATE_CALLTYPE*)(String const* className, String const* methodName);
using MethodBindPtrCallFunc =
	void(CORECLR_DELEGATE_CALLTYPE*)(void const* methodBind, void* objectPtr, void** args, void* retPtr);
using GetConstructorFunc                        = ConstructorFunc(CORECLR_DELEGATE_CALLTYPE*)(String const* name);
using GetObjectClassNameFunc                    = void(CORECLR_DELEGATE_CALLTYPE*)(void* objectPtr, String* outName);
using TieNativeManagedToUnmanagedFunc           = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle, void* native);
using UnmanagedGetInstanceBindingManagedFunc    = void*(CORECLR_DELEGATE_CALLTYPE*)(void* native);
using UnmanagedInstanceBindingCreateManagedFunc = void*(CORECLR_DELEGATE_CALLTYPE*)(void* native);

struct NativeCallbacks
{
	LogFunc                                   m_log                                   = nullptr;
	CreateStringFunc                          m_createString                          = nullptr;
	DestroyStringFunc                         m_destroyString                         = nullptr;
	GetMethodBindFunc                         m_getMethodBind                         = nullptr;
	MethodBindPtrCallFunc                     m_methodBindPtrCall                     = nullptr;
	GetConstructorFunc                        m_getConstructor                        = nullptr;
	GetObjectClassNameFunc                    m_getObjectClassName                    = nullptr;
	TieNativeManagedToUnmanagedFunc           m_tieNativeManagedToUnmanaged           = nullptr;
	UnmanagedGetInstanceBindingManagedFunc    m_unmanagedGetInstanceBindingManaged    = nullptr;
	UnmanagedInstanceBindingCreateManagedFunc m_unmanagedInstanceBindingCreateManaged = nullptr;
};

//---------------------------------------------------------------------------

// Managed callbacks

using AddScriptBridgeFunc             = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* script, String const* scriptPath);
using RemoveScriptBridgeFunc          = int32_t(CORECLR_DELEGATE_CALLTYPE*)(void* script);
using CreateNativeManagedInstanceFunc = void*(CORECLR_DELEGATE_CALLTYPE*)(String const* nativeClassName, void* owner);
using CreateUserManagedInstanceFunc   = void*(CORECLR_DELEGATE_CALLTYPE*)(void* script, void* owner);
using ReleaseGCHandleFunc             = void(CORECLR_DELEGATE_CALLTYPE*)(void* gcHandle);

struct ManagedCallbacks
{
	AddScriptBridgeFunc             m_addScriptBridge             = nullptr;
	RemoveScriptBridgeFunc          m_removeScriptBridge          = nullptr;
	CreateNativeManagedInstanceFunc m_createNativeManagedInstance = nullptr;
	CreateUserManagedInstanceFunc   m_createUserManagedInstance   = nullptr;
	ReleaseGCHandleFunc             m_releaseGCHandle             = nullptr;
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

private:
	// When we reload the assembly(suppose we have already rebuild a new version), we need to
	// 1) Record all the Objects and their script
	// 2) Detach all the script instances from their owners
	// 3) Unload the assembly and load the new one
	// 4) Recreate the script instances and reattach them to their owners
	struct StateBackup
	{
		ObjectID          m_owner;
		Ref<CSharpScript> m_script;
	};

public:
	ScriptSystem(ScriptSystemConfig const& config);

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	bool EnsureProjectSolution();
	bool BuildProjectSolution();
	bool ReloadProjectAssembly();

	bool AddScriptBridge(CSharpScript* script, std::string const& scriptPath);
	bool RemoveScriptBridge(CSharpScript* script);

	void* GetOrCreateNativeManagedWrapper(Object* owner);
	bool  CreateUserManagedInstance(CSharpScript* script, Object* owner);
	void  ReleaseGCHandle(void* gcHandle);

	bool TryBeginScriptInstantiation()
	{
		return !m_scriptExecutionSuspended || std::exchange(m_isRecreatingInstances, false);
	}
	bool IsScriptExecutionSuspended() const { return m_scriptExecutionSuspended; }
	void RegisterScriptOwner(ObjectID id);
	void UnregisterScriptOwner(ObjectID id);

private:
	using ShutdownFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)();
	using LoadProjectAssemblyFunc =
		int32_t(CORECLR_DELEGATE_CALLTYPE*)(String const* assemblyPath, String* outLoadedAssemblyPath);
	using UnloadProjectAssemblyFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)();
	using EnsureProjectSolutionFunc =
		int32_t(CORECLR_DELEGATE_CALLTYPE*)(String const* projectDirectory, String const* sdkDirectory);
	using BuildProjectSolutionFunc = int32_t(CORECLR_DELEGATE_CALLTYPE*)(String const* projectDirectory);

	bool InitializeDotNetRuntime();
	void CollectReloadInstances();
	void DetachScriptInstances();
	bool RestoreScriptInstances();

	bool LoadProjectAssembly();
	bool UnloadProjectAssembly();

private:
	bool                      m_scriptExecutionSuspended = false;
	bool                      m_isRecreatingInstances    = false;
	bool                      m_isInitialized            = false;
	bool                      m_isAssemblyReloading      = false;
	void*                     m_hostfxrModule            = nullptr;
	ShutdownFunc              m_shutdown                 = nullptr;
	LoadProjectAssemblyFunc   m_loadProjectAssembly      = nullptr;
	UnloadProjectAssemblyFunc m_unloadProjectAssembly    = nullptr;
	EnsureProjectSolutionFunc m_ensureProjectSolution    = nullptr;
	BuildProjectSolutionFunc  m_buildProjectSolution     = nullptr;

	ManagedCallbacks m_managedCallbacks = {};

	std::set<ObjectID> m_scriptOwners;

	// Keep the owners and scripts that must be recreated by the next assembly reload.
	// e.g. m_pendingReloadInstances is cleared once every instance is restored.
	std::vector<StateBackup> m_pendingReloadBackups;
};

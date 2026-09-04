#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptInstance.hpp"

#include "ThirdParty/DotNetHost/hostfxr.h"
#include "ThirdParty/DotNetHost/nethost.h"

#include <filesystem>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef GetClassName
#undef GetClassName
#endif

#pragma comment(lib, "ThirdParty/DotNetHost/nethost.lib")

namespace
{
std::filesystem::path GetExecutableDirectory()
{
	std::wstring executablePath(32768, L'\0');

	DWORD const length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));

	if (length == 0 || length >= executablePath.size())
	{
		return {};
	}

	executablePath.resize(length);
	return std::filesystem::path(executablePath).parent_path();
}

int32_t CORECLR_DELEGATE_CALLTYPE LogUtf8(uint8_t const* text, int32_t textLength)
{
	if (textLength < 0)
	{
		return -1;
	}

	if (text == nullptr && textLength != 0)
	{
		return -1;
	}

	std::string const message(reinterpret_cast<char const*>(text), static_cast<size_t>(textLength));

	DebuggerPrintf("[Managed] %s\n", message.c_str());

	return 0;
}

// Return the class name of the given object as a UTF-8 string.
// e.g. GetObjectClassName(nodePtr) -> "Node"
char const* CORECLR_DELEGATE_CALLTYPE GetObjectClassName(void* objectPtr)
{
	if (objectPtr == nullptr)
		return nullptr;

	static std::string classNameBuffer;
	classNameBuffer = static_cast<Object*>(objectPtr)->GetClassName();
	return classNameBuffer.c_str();
}

void const* CORECLR_DELEGATE_CALLTYPE
GetMethodBind(uint8_t const* className, int32_t classNameLength, uint8_t const* methodName, int32_t methodNameLength)
{
	if (className == nullptr || classNameLength <= 0 || methodName == nullptr || methodNameLength <= 0)
	{
		return nullptr;
	}

	std::string const classNameStr(reinterpret_cast<char const*>(className), static_cast<size_t>(classNameLength));
	std::string const methodNameStr(reinterpret_cast<char const*>(methodName), static_cast<size_t>(methodNameLength));

	return ClassDatabase::GetMethodBind(classNameStr.c_str(), methodNameStr.c_str());
}

void CORECLR_DELEGATE_CALLTYPE MethodBindPtrCall(void const* methodBind, void* objectPtr, void** args, void* retPtr)
{
	if (methodBind == nullptr || objectPtr == nullptr)
	{
		return;
	}

	MethodBind const* methodBindPtr = static_cast<MethodBind const*>(methodBind);
	Object*           object        = static_cast<Object*>(objectPtr);
	methodBindPtr->PtrCall(object, args, retPtr);
}

void* CORECLR_DELEGATE_CALLTYPE CreateString(void const* str, int32_t length)
{
	if (str == nullptr || length <= 0)
	{
		return nullptr;
	}

	return new std::string(reinterpret_cast<char const*>(str), static_cast<size_t>(length));
}

void* CORECLR_DELEGATE_CALLTYPE GetStringBuffer(void* str)
{
	if (str == nullptr)
	{
		return nullptr;
	}

	std::string* stringPtr = static_cast<std::string*>(str);
	return (void*)stringPtr->data();
}

int32_t CORECLR_DELEGATE_CALLTYPE GetStringLength(void const* str)
{
	if (str == nullptr)
	{
		return 0;
	}

	std::string const* stringPtr = static_cast<std::string const*>(str);
	return static_cast<int32_t>(stringPtr->size());
}

void CORECLR_DELEGATE_CALLTYPE DestroyString(void const* str)
{
	if (str == nullptr)
	{
		return;
	}

	delete static_cast<std::string const*>(str);
}

int32_t CORECLR_DELEGATE_CALLTYPE TieNativeManagedToUnmanaged(void* gcHandleValue, void* nativeValue)
{
	if (gcHandleValue == nullptr || nativeValue == nullptr)
	{
		return 0;
	}

	Object* owner = static_cast<Object*>(nativeValue);
	return owner->TrySetNativeBindingGCHandle(gcHandleValue) ? 1 : 0;
}

ConstructorFunc CORECLR_DELEGATE_CALLTYPE GetConstructor(void* name)
{
	if (name == nullptr)
	{
		return nullptr;
	}

	MingString const* namePtr = static_cast<MingString const*>(name);
	if (namePtr->m_string == nullptr)
	{
		return nullptr;
	}

	return ClassDatabase::GetConstructor(namePtr->m_string->c_str());
}

void* CORECLR_DELEGATE_CALLTYPE UnmanagedGetInstanceBindingManaged(void* nativeValue)
{
	if (nativeValue == nullptr)
	{
		return nullptr;
	}

	Object* owner = static_cast<Object*>(nativeValue);
	return owner->GetNativeBindingGCHandle();
}

void* CORECLR_DELEGATE_CALLTYPE UnmanagedInstanceBindingCreateManaged(void* nativeValue)
{
	if (nativeValue == nullptr || g_engine == nullptr || g_engine->m_scriptSystem == nullptr)
	{
		return nullptr;
	}

	return g_engine->m_scriptSystem->GetOrCreateNativeManagedWrapper(static_cast<Object*>(nativeValue));
}

} // namespace

ScriptSystem::ScriptSystem([[maybe_unused]] ScriptSystemConfig const& config) {}

void ScriptSystem::Startup()
{
	bool result = InitializeDotNetRuntime();

	GUARANTEE_OR_DIE(result, "Failed to initialize .NET Runtime.");

	RunNativeBindingSmoke();
}

void ScriptSystem::Shutdown()
{
	if (!m_isInitialized)
	{
		return;
	}

	m_isInitialized = false;

	if (m_shutdown != nullptr)
	{
		m_shutdown();
	}

	DebuggerPrintf(".NET Runtime shutdown completed.\n");
}

void ScriptSystem::BeginFrame() {}

void ScriptSystem::EndFrame() {}

void ScriptSystem::ReleaseGCHandle(void* gcHandle)
{
	if (m_isInitialized && m_managedCallbacks.m_releaseGCHandle != nullptr)
	{
		m_managedCallbacks.m_releaseGCHandle(gcHandle);
	}
}

bool ScriptSystem::CreateUserManagedInstance(CSharpScript* script, Object* owner)
{
	if (!m_isInitialized || script == nullptr || owner == nullptr
		|| m_managedCallbacks.m_createUserManagedInstance == nullptr)
	{
		return false;
	}

	void* gcHandle = m_managedCallbacks.m_createUserManagedInstance(script, owner);
	if (gcHandle == nullptr)
	{
		return false;
	}

	ScriptInstance* instance = owner->GetScriptInstance();
	if (instance == nullptr || instance->GetOwner() != owner || !instance->ReloadGCHandle(gcHandle))
	{
		ReleaseGCHandle(gcHandle);
		return false;
	}

	return true;
}

void ScriptSystem::CollectAndGetManagedScriptState(
	int32_t& allocated, int32_t& disposed, int32_t& freed, int32_t& targetAlive)
{
	allocated   = 0;
	disposed    = 0;
	freed       = 0;
	targetAlive = 0;

	GUARANTEE_OR_DIE(
		m_managedCallbacks.m_collectAndGetManagedScriptState != nullptr,
		"Managed script state callback is not initialized.");

	m_managedCallbacks.m_collectAndGetManagedScriptState(&allocated, &disposed, &freed, &targetAlive);
}

bool ScriptSystem::InitializeDotNetRuntime()
{
	std::filesystem::path const executableDirectory = GetExecutableDirectory();

	std::filesystem::path const managedApiDirectory = executableDirectory / L"MingSharp" / L"Api";
	std::filesystem::path const runtimeConfigPath   = managedApiDirectory / L"MingPlugins.runtimeconfig.json";
	std::filesystem::path const managedAssemblyPath = managedApiDirectory / L"MingPlugins.dll";

	if (!std::filesystem::exists(runtimeConfigPath))
	{
		DebuggerPrintf("Missing .NET runtime config: %ls\n", runtimeConfigPath.c_str());
		return false;
	}

	if (!std::filesystem::exists(managedAssemblyPath))
	{
		DebuggerPrintf("Missing managed assembly: %ls\n", managedAssemblyPath.c_str());
		return false;
	}

	size_t hostfxrPathSize = 0;

	int result = get_hostfxr_path(nullptr, &hostfxrPathSize, nullptr);

	if (hostfxrPathSize == 0)
	{
		DebuggerPrintf("Failed to determine hostfxr path size: 0x%08X\n", result);
		return false;
	}

	std::vector<wchar_t> hostfxrPath(hostfxrPathSize);

	result = get_hostfxr_path(hostfxrPath.data(), &hostfxrPathSize, nullptr);

	if (result != 0)
	{
		DebuggerPrintf("Failed to locate hostfxr: 0x%08X\n", result);
		return false;
	}

	m_hostfxrModule = LoadLibraryW(hostfxrPath.data());

	if (m_hostfxrModule == nullptr)
	{
		DebuggerPrintf("Failed to load hostfxr: %ls\n", hostfxrPath.data());
		return false;
	}

	auto initializeForRuntimeConfig = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
		GetProcAddress(static_cast<HMODULE>(m_hostfxrModule), "hostfxr_initialize_for_runtime_config"));

	auto getRuntimeDelegate = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
		GetProcAddress(static_cast<HMODULE>(m_hostfxrModule), "hostfxr_get_runtime_delegate"));

	auto closeHostContext =
		reinterpret_cast<hostfxr_close_fn>(GetProcAddress(static_cast<HMODULE>(m_hostfxrModule), "hostfxr_close"));

	if (initializeForRuntimeConfig == nullptr || getRuntimeDelegate == nullptr || closeHostContext == nullptr)
	{
		DebuggerPrintf("Failed to load required hostfxr exports.\n");
		return false;
	}

	hostfxr_handle hostContext = nullptr;

	result = initializeForRuntimeConfig(runtimeConfigPath.c_str(), nullptr, &hostContext);

	if (result != 0 || hostContext == nullptr)
	{
		DebuggerPrintf("Failed to initialize .NET host context: 0x%08X\n", result);

		if (hostContext != nullptr)
		{
			closeHostContext(hostContext);
		}

		return false;
	}

	load_assembly_and_get_function_pointer_fn loadAssembly = nullptr;

	result = getRuntimeDelegate(
		hostContext,
		hdt_load_assembly_and_get_function_pointer,
		reinterpret_cast<void**>(&loadAssembly));

	closeHostContext(hostContext);
	hostContext = nullptr;

	if (result != 0 || loadAssembly == nullptr)
	{
		DebuggerPrintf("Failed to get .NET assembly loader: 0x%08X\n", result);
		return false;
	}

	wchar_t const* typeName   = L"MingPlugins.Main, MingPlugins";
	InitializeFunc initialize = nullptr;

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"Initialize",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&initialize));

	if (result != 0 || initialize == nullptr)
	{
		DebuggerPrintf("Failed to load managed Initialize: 0x%08X\n", result);
		return false;
	}

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"Shutdown",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_shutdown));

	if (result != 0 || m_shutdown == nullptr)
	{
		DebuggerPrintf("Failed to load managed Shutdown: 0x%08X\n", result);
		return false;
	}

	NativeCallbacks const nativeCallbacks{
		&LogUtf8,
		&GetObjectClassName,
		&GetMethodBind,
		&MethodBindPtrCall,
		&CreateString,
		&GetStringBuffer,
		&GetStringLength,
		&DestroyString,
		&TieNativeManagedToUnmanaged,
		&GetConstructor,
		&UnmanagedGetInstanceBindingManaged,
		&UnmanagedInstanceBindingCreateManaged,
	};

	int32_t const initResult = initialize(
		&nativeCallbacks,
		static_cast<int32_t>(sizeof(nativeCallbacks)),
		&m_managedCallbacks,
		static_cast<int32_t>(sizeof(m_managedCallbacks)));
	if (initResult != 0)
	{
		DebuggerPrintf("Managed Initialize failed: 0x%08X\n", initResult);
		return false;
	}

	m_isInitialized = true;

	DebuggerPrintf(".NET Runtime initialized.\n");

	return true;
}

void* ScriptSystem::GetOrCreateNativeManagedWrapper(Object* owner)
{
	if (!m_isInitialized || owner == nullptr)
	{
		return nullptr;
	}

	if (owner->IsNativeBindingGCHandleValid())
	{
		return owner->GetNativeBindingGCHandle();
	}

	std::string nativeClassName = owner->GetClassName();
	MingString  nativeClassNameStruct;
	nativeClassNameStruct.m_string = &nativeClassName;

	if (m_managedCallbacks.m_createNativeManagedInstance == nullptr)
	{
		return nullptr;
	}

	void* gcHandle = m_managedCallbacks.m_createNativeManagedInstance(&nativeClassNameStruct, owner);
	if (gcHandle == nullptr)
	{
		return nullptr;
	}

	if (!owner->TrySetNativeBindingGCHandle(gcHandle))
	{
		ReleaseGCHandle(gcHandle);
		return owner->GetNativeBindingGCHandle();
	}

	return gcHandle;
}

void ScriptSystem::RunNativeBindingSmoke()
{
	GUARANTEE_OR_DIE(
		m_managedCallbacks.m_createNativeManagedWrapperForSmoke != nullptr
			&& m_managedCallbacks.m_validateNativeManagedWrapper != nullptr
			&& m_managedCallbacks.m_collectAndGetNativeBindingState != nullptr,
		"Native binding smoke callbacks are not initialized.");

	auto collectState = [this](int32_t& allocated, int32_t& disposed, int32_t& freed) {
		m_managedCallbacks.m_collectAndGetNativeBindingState(&allocated, &disposed, &freed);
	};

	int32_t allocatedBefore;
	int32_t disposedBefore;
	int32_t freedBefore;
	collectState(allocatedBefore, disposedBefore, freedBefore);

	Object* managedFirstOwner = static_cast<Object*>(m_managedCallbacks.m_createNativeManagedWrapperForSmoke());
	GUARANTEE_OR_DIE(
		managedFirstOwner != nullptr && managedFirstOwner->IsNativeBindingGCHandleValid()
			&& m_managedCallbacks.m_validateNativeManagedWrapper(managedFirstOwner) != 0,
		"C#-first native binding identity smoke failed.");
	MemDelete(managedFirstOwner);

	int32_t allocatedAfterManagedFirst;
	int32_t disposedAfterManagedFirst;
	int32_t freedAfterManagedFirst;
	collectState(allocatedAfterManagedFirst, disposedAfterManagedFirst, freedAfterManagedFirst);
	GUARANTEE_OR_DIE(
		allocatedAfterManagedFirst == allocatedBefore + 1 && disposedAfterManagedFirst == disposedBefore + 1
			&& freedAfterManagedFirst == freedBefore + 1,
		"C#-first native binding release smoke failed.");

	Object* nativeFirstOwner = ClassDatabase::CreateInstance("Node3D");
	GUARANTEE_OR_DIE(
		nativeFirstOwner != nullptr && !nativeFirstOwner->IsNativeBindingGCHandleValid(),
		"Native object unexpectedly created a managed wrapper during initialization.");
	GUARANTEE_OR_DIE(
		m_managedCallbacks.m_validateNativeManagedWrapper(nativeFirstOwner) != 0
			&& nativeFirstOwner->IsNativeBindingGCHandleValid(),
		"Native-first lazy binding identity smoke failed.");
	MemDelete(nativeFirstOwner);

	int32_t allocatedAfterNativeFirst;
	int32_t disposedAfterNativeFirst;
	int32_t freedAfterNativeFirst;
	collectState(allocatedAfterNativeFirst, disposedAfterNativeFirst, freedAfterNativeFirst);
	GUARANTEE_OR_DIE(
		allocatedAfterNativeFirst == allocatedAfterManagedFirst + 1
			&& disposedAfterNativeFirst == disposedAfterManagedFirst + 1
			&& freedAfterNativeFirst == freedAfterManagedFirst + 1,
		"Native-first native binding release smoke failed.");

	DebuggerPrintf("Native managed wrapper identity smoke passed.\n");
}

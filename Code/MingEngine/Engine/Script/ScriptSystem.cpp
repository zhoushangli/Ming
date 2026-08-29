#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
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

// Create an object from the class database by class name and return its pointer.
// e.g. CreateObject("Node")
void* CORECLR_DELEGATE_CALLTYPE CreateObject(char const* className)
{
	if (className == nullptr)
		return nullptr;
	return ClassDatabase::CreateInstance(className);
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

int32_t CORECLR_DELEGATE_CALLTYPE BindManagedScriptInstance(void* ownerValue, void* gcHandleValue)
{
	if (ownerValue == nullptr || gcHandleValue == nullptr)
	{
		return 0;
	}

	Object* owner = static_cast<Object*>(ownerValue);

	ScriptInstance* instance = dynamic_cast<ScriptInstance*>(owner->GetScriptInstance());

	if (instance == nullptr || instance->GetOwner() != owner)
	{
		return 0;
	}

	return instance->ReloadGCHandle(gcHandleValue) ? 1 : 0;
}
} // namespace

ScriptSystem::ScriptSystem([[maybe_unused]] ScriptSystemConfig const& config) {}

void ScriptSystem::Startup()
{
	bool result = InitializeDotNetRuntime();

	GUARANTEE_OR_DIE(result, "Failed to initialize .NET Runtime.");
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

void ScriptSystem::FreeGCHandle(void* gcHandle)
{
	if (m_isInitialized && m_managedCallbacks.m_freeGCHandle != nullptr)
	{
		m_managedCallbacks.m_freeGCHandle(gcHandle);
	}
}

bool ScriptSystem::CreateManagedScriptInstance(Script* script, Object* owner)
{
	int const result = m_managedCallbacks.m_createManagedScriptInstance(script, owner);
	return result != 0;
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
		&LogUtf8,      &CreateObject,    &GetObjectClassName, &GetMethodBind, &MethodBindPtrCall,
		&CreateString, &GetStringBuffer, &GetStringLength,    &DestroyString, &BindManagedScriptInstance
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

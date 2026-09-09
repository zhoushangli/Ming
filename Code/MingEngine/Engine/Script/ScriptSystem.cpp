#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/MethodBind.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptInstance.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Script/CSharpScript.hpp"

#define NETHOST_USE_AS_STATIC
#include "ThirdParty/DotNetHost/hostfxr.h"
#include "ThirdParty/DotNetHost/nethost.h"

#include <filesystem>
#include <limits>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef GetClassName
#undef GetClassName
#endif

#pragma comment(lib, "ThirdParty/DotNetHost/libnethost.lib")

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
	// 1) Initialize the runtime and managed bridge
	bool const initialized = InitializeDotNetRuntime();

	GUARANTEE_OR_DIE(initialized, "Failed to initialize .NET Runtime or managed bridge.");

	// 2) Load the current project's assembly
	if (!LoadProjectAssembly())
	{
		DebuggerPrintf("Continuing startup without a loaded project assembly.\n");
	}
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

		m_shutdown              = nullptr;
		m_loadProjectAssembly   = nullptr;
		m_ensureProjectSolution = nullptr;
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

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"LoadProjectAssembly",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_loadProjectAssembly));

	if (result != 0 || m_loadProjectAssembly == nullptr)
	{
		DebuggerPrintf(
			"Failed to resolve managed entry point: LoadProjectAssembly "
			"(0x%08X)\n",
			result);
		return false;
	}

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"EnsureProjectSolution",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_ensureProjectSolution));

	if (result != 0 || m_ensureProjectSolution == nullptr)
	{
		DebuggerPrintf(
			"Failed to resolve managed entry point: EnsureProjectSolution "
			"(0x%08X)\n",
			result);
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

bool ScriptSystem::LoadProjectAssembly()
{
	// 1) Check the runtime and managed entry point
	if (!m_isInitialized || m_loadProjectAssembly == nullptr)
	{
		DebuggerPrintf("Cannot load project assembly: managed bridge is not initialized.\n");
		return false;
	}

	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		DebuggerPrintf("Cannot load project assembly: file system is unavailable.\n");
		return false;
	}

	// 2) Resolve the current project's assembly path
	VirtualPath const virtualPath("res://.ming/dotnet/bin/Debug/Game.dll");

	std::filesystem::path assemblyPath;

	if (!g_engine->m_fileSystem->TryGetPhysicalPath(virtualPath, assemblyPath))
	{
		DebuggerPrintf("Failed to resolve project assembly path: %s\n", virtualPath.CStr());
		return false;
	}

	DebuggerPrintf("Project assembly expected path: %ls\n", assemblyPath.c_str());

	// 3) Check whether the assembly file exists
	std::error_code errorCode;
	bool const      exists = std::filesystem::exists(assemblyPath, errorCode);

	if (errorCode)
	{
		DebuggerPrintf(
			"Failed to inspect project assembly: %ls (%s)\n",
			assemblyPath.c_str(),
			errorCode.message().c_str());
		return false;
	}

	if (!exists)
	{
		DebuggerPrintf(
			"Project assembly does not exist: %ls\n"
			"Build Game.csproj with configuration Debug first.\n",
			assemblyPath.c_str());
		return false;
	}

	// 4) Invoke the managed loader
	MingString nativeLoadedPath{};

	int32_t const result = m_loadProjectAssembly(assemblyPath.c_str(), &nativeLoadedPath);

	// 5) Handle failure and release any returned string
	if (result != 0)
	{
		DestroyString(nativeLoadedPath.m_string);
		nativeLoadedPath.m_string = nullptr;

		DebuggerPrintf("Project assembly load did not succeed: %ls (status: %d)\n", assemblyPath.c_str(), result);
		return false;
	}

	// 6) Validate the actual loaded path
	if (nativeLoadedPath.m_string == nullptr || nativeLoadedPath.m_string->empty())
	{
		DestroyString(nativeLoadedPath.m_string);
		nativeLoadedPath.m_string = nullptr;

		DebuggerPrintf("Managed loader returned success without an assembly path.\n");
		return false;
	}

	// 7) Take the path value and release the native string object
	std::string loadedPath = std::move(*nativeLoadedPath.m_string);

	DestroyString(nativeLoadedPath.m_string);
	nativeLoadedPath.m_string = nullptr;

	DebuggerPrintf("Project assembly confirmed by C++: %s\n", loadedPath.c_str());

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

bool ScriptSystem::AddScriptBridge(CSharpScript* script, std::string const& scriptPath)
{
	if (!m_isInitialized || script == nullptr || scriptPath.empty() || m_managedCallbacks.m_addScriptBridge == nullptr
		|| scriptPath.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()))
	{
		return false;
	}

	return m_managedCallbacks.m_addScriptBridge(
			   script,
			   reinterpret_cast<uint8_t const*>(scriptPath.data()),
			   static_cast<int32_t>(scriptPath.size()))
		   == 1;
}

bool ScriptSystem::RemoveScriptBridge(CSharpScript* script)
{
	if (!m_isInitialized || script == nullptr || m_managedCallbacks.m_removeScriptBridge == nullptr)
	{
		return false;
	}

	return m_managedCallbacks.m_removeScriptBridge(script) == 1;
}

bool ScriptSystem::EnsureProjectSolution()
{
	// 1) Check the managed entry point
	if (!m_isInitialized || m_ensureProjectSolution == nullptr)
	{
		DebuggerPrintf(
			"Cannot ensure C# project files: "
			"managed bridge is not initialized.\n");

		return false;
	}

	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		DebuggerPrintf(
			"Cannot ensure C# project files: "
			"file system is unavailable.\n");

		return false;
	}

	// 2) Resolve the active project directory
	std::filesystem::path projectDirectory;

	if (!g_engine->m_fileSystem->TryGetPhysicalPath(VirtualPath("res://"), projectDirectory))
	{
		DebuggerPrintf("Failed to resolve the active project directory.\n");

		return false;
	}

	// 3) Resolve the SDK directory beside the executable
	std::filesystem::path const executableDirectory = GetExecutableDirectory();

	if (executableDirectory.empty())
	{
		DebuggerPrintf("Failed to resolve the executable directory.\n");

		return false;
	}

	std::filesystem::path const sdkDirectory = executableDirectory / L"MingSharp" / L"Tool" / L"Sdk";

	// 4) Call the managed generator
	int32_t const result = m_ensureProjectSolution(projectDirectory.c_str(), sdkDirectory.c_str());

	if (result != 0)
	{
		DebuggerPrintf("Failed to ensure C# project files: %d\n", result);

		return false;
	}

	DebuggerPrintf("C# project files are ready: %ls\n", projectDirectory.c_str());

	return true;
}

#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/MethodBind.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptInstance.hpp"
#include "MingEngine/Core/StringUtils.hpp"
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

	std::string const message(
		textLength == 0 ? "" : reinterpret_cast<char const*>(text),
		static_cast<size_t>(textLength));

	if (message.compare(0, 8, "[Error] ") == 0)
	{
		ERR_PRINT(message.substr(8));
	}
	else if (message.compare(0, 10, "[Warning] ") == 0)
	{
		WARN_PRINT(message.substr(10));
	}
	else
	{
		INFO_PRINT(message);
	}

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
		WARN_PRINT("Continuing startup without a loaded project assembly.\n");
	}
}

void ScriptSystem::Shutdown()
{
	if (!m_isInitialized)
	{
		return;
	}

	// 1) Release retained resources while the managed bridge is still available
	DetachScriptInstances();
	m_pendingReloadState.clear();
	UnloadProjectAssembly();

	// 2) Release the managed entry points
	m_isInitialized = false;

	if (m_shutdown != nullptr)
	{
		m_shutdown();

		m_shutdown              = nullptr;
		m_loadProjectAssembly   = nullptr;
		m_unloadProjectAssembly = nullptr;
		m_ensureProjectSolution = nullptr;
		m_buildProjectSolution  = nullptr;
	}

	INFO_PRINT("Script system shutdown completed.\n");
}

void ScriptSystem::BeginFrame() {}

void ScriptSystem::EndFrame()
{
	// Reload the current project assembly and restore its script instances.
	// e.g. Press F5 after rebuilding Game.dll.
	if (m_isInitialized && (GetAsyncKeyState(VK_F5) & 0x1) != 0)
	{
		ReloadProjectAssembly();
	}
}

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

bool ScriptSystem::ValidateManagedScriptInstance(void* gcHandle, Object* expectedOwner)
{
	if (gcHandle == nullptr || expectedOwner == nullptr
		|| m_managedCallbacks.m_validateManagedScriptInstance == nullptr)
	{
		return false;
	}

	return m_managedCallbacks.m_validateManagedScriptInstance(gcHandle, expectedOwner) != 0;
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

	ERR_FAIL_COND_V_MSG(
		!std::filesystem::exists(runtimeConfigPath),
		false,
		Stringf("Missing .NET runtime config: %ls\n", runtimeConfigPath.c_str()));

	ERR_FAIL_COND_V_MSG(
		!std::filesystem::exists(managedAssemblyPath),
		false,
		Stringf("Missing managed assembly: %ls\n", managedAssemblyPath.c_str()));

	size_t hostfxrPathSize = 0;

	int result = get_hostfxr_path(nullptr, &hostfxrPathSize, nullptr);

	ERR_FAIL_COND_V_MSG(
		hostfxrPathSize == 0,
		false,
		Stringf("Failed to determine hostfxr path size: 0x%08X\n", result));

	std::vector<wchar_t> hostfxrPath(hostfxrPathSize);

	result = get_hostfxr_path(hostfxrPath.data(), &hostfxrPathSize, nullptr);

	ERR_FAIL_COND_V_MSG(result != 0, false, Stringf("Failed to locate hostfxr: 0x%08X\n", result));

	m_hostfxrModule = LoadLibraryW(hostfxrPath.data());

	ERR_FAIL_COND_V_MSG(
		m_hostfxrModule == nullptr,
		false,
		Stringf("Failed to load hostfxr: %ls\n", hostfxrPath.data()));

	auto initializeForRuntimeConfig = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
		GetProcAddress(static_cast<HMODULE>(m_hostfxrModule), "hostfxr_initialize_for_runtime_config"));

	auto getRuntimeDelegate = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
		GetProcAddress(static_cast<HMODULE>(m_hostfxrModule), "hostfxr_get_runtime_delegate"));

	auto closeHostContext =
		reinterpret_cast<hostfxr_close_fn>(GetProcAddress(static_cast<HMODULE>(m_hostfxrModule), "hostfxr_close"));

	ERR_FAIL_COND_V_MSG(
		initializeForRuntimeConfig == nullptr || getRuntimeDelegate == nullptr || closeHostContext == nullptr,
		false,
		"Failed to load required hostfxr exports.\n");

	hostfxr_handle hostContext = nullptr;

	result = initializeForRuntimeConfig(runtimeConfigPath.c_str(), nullptr, &hostContext);

	if (result != 0 || hostContext == nullptr)
	{
		ERR_PRINT(Stringf("Failed to initialize .NET host context: 0x%08X\n", result));

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

	ERR_FAIL_COND_V_MSG(
		result != 0 || loadAssembly == nullptr,
		false,
		Stringf("Failed to get .NET assembly loader: 0x%08X\n", result));

	wchar_t const* typeName   = L"MingPlugins.Main, MingPlugins";
	InitializeFunc initialize = nullptr;

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"Initialize",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&initialize));

	ERR_FAIL_COND_V_MSG(
		result != 0 || initialize == nullptr,
		false,
		Stringf("Failed to load managed Initialize: 0x%08X\n", result));

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"Shutdown",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_shutdown));

	ERR_FAIL_COND_V_MSG(
		result != 0 || m_shutdown == nullptr,
		false,
		Stringf("Failed to load managed Shutdown: 0x%08X\n", result));

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"LoadProjectAssembly",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_loadProjectAssembly));

	ERR_FAIL_COND_V_MSG(
		result != 0 || m_loadProjectAssembly == nullptr,
		false,
		Stringf(
			"Failed to resolve managed entry point: LoadProjectAssembly "
			"(0x%08X)\n",
			result));

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"UnloadProjectAssembly",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_unloadProjectAssembly));

	ERR_FAIL_COND_V_MSG(
		result != 0 || m_unloadProjectAssembly == nullptr,
		false,
		Stringf(
			"Failed to resolve managed entry point: UnloadProjectAssembly "
			"(0x%08X)\n",
			result));

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"EnsureProjectSolution",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_ensureProjectSolution));

	ERR_FAIL_COND_V_MSG(
		result != 0 || m_ensureProjectSolution == nullptr,
		false,
		Stringf(
			"Failed to resolve managed entry point: EnsureProjectSolution "
			"(0x%08X)\n",
			result));

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"BuildProjectSolution",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&m_buildProjectSolution));

	ERR_FAIL_COND_V_MSG(
		result != 0 || m_buildProjectSolution == nullptr,
		false,
		Stringf(
			"Failed to resolve managed entry point: BuildProjectSolution "
			"(0x%08X)\n",
			result));

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
	ERR_FAIL_COND_V_MSG(initResult != 0, false, Stringf("Managed Initialize failed: 0x%08X\n", initResult));

	m_isInitialized = true;

	INFO_PRINT(".NET Runtime initialized.\n");

	return true;
}

bool ScriptSystem::LoadProjectAssembly()
{
	// 1) Check the runtime and managed entry point
	ERR_FAIL_COND_V_MSG(
		!m_isInitialized || m_loadProjectAssembly == nullptr,
		false,
		"Cannot load project assembly: managed bridge is not initialized.\n");

	ERR_FAIL_COND_V_MSG(
		g_engine == nullptr || g_engine->m_fileSystem == nullptr,
		false,
		"Cannot load project assembly: file system is unavailable.\n");

	// 2) Resolve the current project's assembly path
	VirtualPath const virtualPath("res://.ming/dotnet/bin/Debug/Game.dll");

	std::filesystem::path assemblyPath;

	ERR_FAIL_COND_V_MSG(
		!g_engine->m_fileSystem->TryGetPhysicalPath(virtualPath, assemblyPath),
		false,
		Stringf("Failed to resolve project assembly path: %s\n", virtualPath.CStr()));

	// 3) Check whether the assembly file exists
	std::error_code errorCode;
	bool const      exists = std::filesystem::exists(assemblyPath, errorCode);

	ERR_FAIL_COND_V_MSG(
		errorCode,
		false,
		Stringf("Failed to inspect project assembly: %ls (%s)\n", assemblyPath.c_str(), errorCode.message().c_str()));

	ERR_FAIL_COND_V_MSG(
		!exists,
		false,
		Stringf(
			"Project assembly does not exist: %ls\n"
			"Build Game.csproj with configuration Debug first.\n",
			assemblyPath.c_str()));

	// 4) Invoke the managed loader
	MingString nativeLoadedPath{};

	int32_t const result = m_loadProjectAssembly(assemblyPath.c_str(), &nativeLoadedPath);

	// 5) Handle failure and release any returned string
	if (result != 0)
	{
		DestroyString(nativeLoadedPath.m_string);
		nativeLoadedPath.m_string = nullptr;

		ERR_PRINT(Stringf("Project assembly load did not succeed: %ls (status: %d)\n", assemblyPath.c_str(), result));
		return false;
	}

	// 6) Validate the actual loaded path
	if (nativeLoadedPath.m_string == nullptr || nativeLoadedPath.m_string->empty())
	{
		DestroyString(nativeLoadedPath.m_string);
		nativeLoadedPath.m_string = nullptr;

		ERR_PRINT("Managed loader returned success without an assembly path.\n");
		return false;
	}

	// 7) Take the path value and release the native string object
	std::string loadedPath = std::move(*nativeLoadedPath.m_string);

	DestroyString(nativeLoadedPath.m_string);
	nativeLoadedPath.m_string = nullptr;

	INFO_PRINT(Stringf("Project assembly loaded: %s\n", loadedPath.c_str()));

	return true;
}

bool ScriptSystem::UnloadProjectAssembly()
{
	// 1) Check the managed entry point
	ERR_FAIL_COND_V_MSG(
		!m_isInitialized || m_unloadProjectAssembly == nullptr,
		false,
		"Cannot unload project assembly: managed bridge is not initialized.\n");

	// 2) Ask the managed side to unload its context
	int32_t const result = m_unloadProjectAssembly();

	ERR_FAIL_COND_V_MSG(result != 0, false, Stringf("Failed to unload the project assembly (status: %d)\n", result));

	INFO_PRINT("Project assembly unloaded.\n");
	return true;
}

bool ScriptSystem::SaveScriptStates()
{
	std::vector<StateBackup> states;
	auto const owners = m_scriptOwners;
	for (ObjectID id : owners)
	{
		Object* owner = ObjectDatabase::GetInstance(id);
		auto* instance = owner ? dynamic_cast<CSharpInstance*>(owner->GetScriptInstance()) : nullptr;
		if (instance == nullptr)
		{
			continue;
		}
		StateBackup backup{id, Ref<CSharpScript>(static_cast<CSharpScript*>(instance->GetScript().Get())), {}};
		MingString state{};
		int32_t const result = m_managedCallbacks.m_serializeScriptState(instance->GetGCHandle(), &state);
		if (result != 0 && state.m_string != nullptr)
		{
			backup.m_state = std::move(*state.m_string);
		}
		DestroyString(state.m_string);
		ERR_FAIL_COND_V_MSG(result == 0 || backup.m_state.empty(), false, "Failed to save script reload state.\n");
		states.push_back(std::move(backup));
	}
	m_pendingReloadState = std::move(states);
	return true;
}

void ScriptSystem::DetachScriptInstances()
{
	for (auto const& backup : m_pendingReloadState)
	{
		Object* owner = ObjectDatabase::GetInstance(backup.m_owner);
		if (owner != nullptr && owner->GetScriptInstance() != nullptr
			&& owner->GetScriptInstance()->GetScript().Get() == backup.m_script.Get())
		{
			owner->SetScriptInstance(nullptr);
		}
	}
}

bool ScriptSystem::RestoreScriptInstances()
{
	// 1) Recreate every instance before restoring any state
	for (auto const& backup : m_pendingReloadState)
	{
		Object* owner = ObjectDatabase::GetInstance(backup.m_owner);
		if (owner == nullptr || owner->GetScriptInstance() != nullptr)
		{
			continue;
		}
		m_isRecreatingInstances = true;
		bool const created = backup.m_script->Instantiate(owner);
		m_isRecreatingInstances = false;
		ERR_FAIL_COND_V_MSG(!created, false, "Failed to recreate script; reload state retained.\n");
	}

	// 2) Keep all original snapshots until the whole restore succeeds
	for (auto const& backup : m_pendingReloadState)
	{
		Object* owner = ObjectDatabase::GetInstance(backup.m_owner);
		auto* instance = owner ? dynamic_cast<CSharpInstance*>(owner->GetScriptInstance()) : nullptr;
		if (instance == nullptr || instance->GetScript().Get() != backup.m_script.Get())
		{
			continue;
		}
		ERR_FAIL_COND_V_MSG(backup.m_state.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()),
			false, "Script reload state is too large.\n");
		int32_t const result = m_managedCallbacks.m_deserializeScriptState(instance->GetGCHandle(),
			reinterpret_cast<uint8_t const*>(backup.m_state.data()), static_cast<int32_t>(backup.m_state.size()));
		ERR_FAIL_COND_V_MSG(result == 0, false, "Failed to restore script; reload state retained.\n");
	}
	return true;
}

bool ScriptSystem::ReloadProjectAssembly()
{
	ERR_FAIL_COND_V_MSG(m_isAssemblyReloading || !m_isInitialized, false, "Cannot start script reload.\n");
	m_isAssemblyReloading = true;
	bool const wasSuspended = m_scriptExecutionSuspended;
	m_scriptExecutionSuspended = true;
	bool success = false;
	bool detached = wasSuspended;
	try
	{
		// 1) Preserve the first snapshot across failed reload attempts
		if (!wasSuspended && !SaveScriptStates())
		{
			m_scriptExecutionSuspended = false;
		}
		else
		{
			// 2) Release old handles before unloading and replace partial retries
			detached = true;
			DetachScriptInstances();
			if (UnloadProjectAssembly() && LoadProjectAssembly() && RestoreScriptInstances())
			{
				m_pendingReloadState.clear();
				m_scriptExecutionSuspended = false;
				success = true;
			}
		}
	}
	catch (std::exception const& exception)
	{
		ERR_PRINT(Stringf("Script reload failed: %s\n", exception.what()));
	}
	if (!detached && !success)
	{
		m_pendingReloadState.clear();
		m_scriptExecutionSuspended = false;
	}
	m_isRecreatingInstances = false;
	m_isAssemblyReloading = false;
	if (success)
	{
		INFO_PRINT("Project assembly and script states reloaded.\n");
	}
	return success;
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

bool ScriptSystem::BuildProjectSolution()
{
	// 1) Check the managed entry point and file system
	ERR_FAIL_COND_V_MSG(
		!m_isInitialized || m_buildProjectSolution == nullptr,
		false,
		"Cannot build C# project: managed bridge is not initialized.\n");

	ERR_FAIL_COND_V_MSG(
		g_engine == nullptr || g_engine->m_fileSystem == nullptr,
		false,
		"Cannot build C# project: file system is unavailable.\n");

	// 2) Resolve the active project directory
	std::filesystem::path projectDirectory;
	ERR_FAIL_COND_V_MSG(
		!g_engine->m_fileSystem->TryGetPhysicalPath(VirtualPath("res://"), projectDirectory),
		false,
		"Failed to resolve the active project directory.\n");

	// 3) Build the solution through the managed entry point
	int32_t const result = m_buildProjectSolution(projectDirectory.c_str());
	ERR_FAIL_COND_V_MSG(
		result != 0,
		false,
		Stringf("Failed to build C# project solution: %ls (status: %d)\n", projectDirectory.c_str(), result));

	INFO_PRINT(Stringf("C# project solution built: %ls\n", projectDirectory.c_str()));
	return true;
}

bool ScriptSystem::EnsureProjectSolution()
{
	// 1) Check the managed entry point
	ERR_FAIL_COND_V_MSG(
		!m_isInitialized || m_ensureProjectSolution == nullptr,
		false,
		"Cannot ensure C# project files: "
		"managed bridge is not initialized.\n");

	ERR_FAIL_COND_V_MSG(
		g_engine == nullptr || g_engine->m_fileSystem == nullptr,
		false,
		"Cannot ensure C# project files: "
		"file system is unavailable.\n");

	// 2) Resolve the active project directory
	std::filesystem::path projectDirectory;

	ERR_FAIL_COND_V_MSG(
		!g_engine->m_fileSystem->TryGetPhysicalPath(VirtualPath("res://"), projectDirectory),
		false,
		"Failed to resolve the active project directory.\n");

	// 3) Resolve the SDK directory beside the executable
	std::filesystem::path const executableDirectory = GetExecutableDirectory();

	ERR_FAIL_COND_V_MSG(executableDirectory.empty(), false, "Failed to resolve the executable directory.\n");

	std::filesystem::path const sdkDirectory = executableDirectory / L"MingSharp" / L"Tool" / L"Sdk";

	// 4) Call the managed generator
	int32_t const result = m_ensureProjectSolution(projectDirectory.c_str(), sdkDirectory.c_str());

	ERR_FAIL_COND_V_MSG(result != 0, false, Stringf("Failed to ensure C# project files: %d\n", result));

	INFO_PRINT(Stringf("C# project files are ready: %ls\n", projectDirectory.c_str()));

	return true;
}

void ScriptSystem::RegisterScriptOwner(ObjectID id) { m_scriptOwners.insert(id); }

void ScriptSystem::UnregisterScriptOwner(ObjectID id) { m_scriptOwners.erase(id); }

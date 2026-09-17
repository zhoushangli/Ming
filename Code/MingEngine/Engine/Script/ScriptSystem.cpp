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

// Borrow the text for this call without transferring ownership.
// e.g. Log(&message) leaves the caller's string intact.
int32_t CORECLR_DELEGATE_CALLTYPE Log(String const* text)
{
	std::string const message = text == nullptr ? std::string() : text->ToUtf8();
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

// Copy borrowed UTF-32 code points into a String owned by the managed caller.
// e.g. CreateString(codes, 4, outString) leaves outString owning 4 code points.
void CORECLR_DELEGATE_CALLTYPE CreateString(char32_t const* str, int32_t length, String* outString)
{
	if (outString == nullptr)
	{
		return;
	}

	if (str == nullptr || length <= 0)
	{
		*outString = String();
		return;
	}

	*outString = String(str, static_cast<uint32_t>(length));
}

// Drop the reference the given managed side string holds and leave it empty.
// e.g. DestroyString(&text) empties text and frees its block when it was the last owner.
void CORECLR_DELEGATE_CALLTYPE DestroyString(String* str)
{
	if (str == nullptr)
	{
		return;
	}

	*str = String();
}

void const* CORECLR_DELEGATE_CALLTYPE GetMethodBind(String const* className, String const* methodName)
{
	if (className == nullptr || methodName == nullptr)
		return nullptr;
	return ClassDatabase::GetMethodBind(className->ToUtf8(), methodName->ToUtf8());
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

ConstructorFunc CORECLR_DELEGATE_CALLTYPE GetConstructor(String const* name)
{
	if (name == nullptr)
		return nullptr;
	return ClassDatabase::GetConstructor(name->ToUtf8());
}

// Transfer a newly allocated class name to the caller.
// e.g. GetObjectClassName(owner, &name) leaves name owning its UTF-32 buffer.
void CORECLR_DELEGATE_CALLTYPE GetObjectClassName(void* objectPtr, String* outName)
{
	if (outName == nullptr)
		return;
	*outName = String();
	if (objectPtr != nullptr)
		*outName = String(static_cast<Object*>(objectPtr)->GetClassName());
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
	m_pendingReloadBackups.clear();
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

void ScriptSystem::EndFrame() {}

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
		&Log,
		&CreateString,
		&DestroyString,
		&GetMethodBind,
		&MethodBindPtrCall,
		&GetConstructor,
		&GetObjectClassName,
		&TieNativeManagedToUnmanaged,
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
	auto const    projectUtf8 = projectDirectory.u8string();
	auto const    sdkUtf8     = sdkDirectory.u8string();
	std::string   projectText(projectUtf8.begin(), projectUtf8.end());
	std::string   sdkText(sdkUtf8.begin(), sdkUtf8.end());
	String const  nativeProject(projectText);
	String const  nativeSdk(sdkText);
	int32_t const result = m_ensureProjectSolution(&nativeProject, &nativeSdk);

	ERR_FAIL_COND_V_MSG(result != 0, false, Stringf("Failed to ensure C# project files: %d\n", result));

	INFO_PRINT(Stringf("C# project files are ready: %ls\n", projectDirectory.c_str()));

	return true;
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
	auto const    pathUtf8 = projectDirectory.u8string();
	std::string   pathText(pathUtf8.begin(), pathUtf8.end());
	String const  nativePath(pathText);
	int32_t const result = m_buildProjectSolution(&nativePath);
	ERR_FAIL_COND_V_MSG(
		result != 0,
		false,
		Stringf("Failed to build C# project solution: %ls (status: %d)\n", projectDirectory.c_str(), result));

	INFO_PRINT(Stringf("C# project solution built: %ls\n", projectDirectory.c_str()));
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
	String nativeLoadedPath{};

	auto const    pathUtf8 = assemblyPath.u8string();
	std::string   pathText(pathUtf8.begin(), pathUtf8.end());
	String const  nativePath(pathText);
	int32_t const result = m_loadProjectAssembly(&nativePath, &nativeLoadedPath);

	// 5) Handle failure and release any returned string
	if (result != 0)
	{
		nativeLoadedPath = String();

		ERR_PRINT(Stringf("Project assembly load did not succeed: %ls (status: %d)\n", assemblyPath.c_str(), result));
		return false;
	}

	// 6) Validate the actual loaded path
	if (nativeLoadedPath.IsEmpty())
	{
		nativeLoadedPath = String();

		ERR_PRINT("Managed loader returned success without an assembly path.\n");
		return false;
	}

	// 7) Take the path value and release the native string object
	std::string loadedPath = nativeLoadedPath.ToUtf8();

	nativeLoadedPath = String();

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

bool ScriptSystem::AddScriptBridge(CSharpScript* script, std::string const& scriptPath)
{
	if (!m_isInitialized || script == nullptr || scriptPath.empty() || m_managedCallbacks.m_addScriptBridge == nullptr
		|| scriptPath.size() > static_cast<size_t>((std::numeric_limits<uint32_t>::max)()))
	{
		return false;
	}

	String const path(scriptPath);
	return m_managedCallbacks.m_addScriptBridge(script, &path) == 1;
}

bool ScriptSystem::RemoveScriptBridge(CSharpScript* script)
{
	if (!m_isInitialized || script == nullptr || m_managedCallbacks.m_removeScriptBridge == nullptr)
	{
		return false;
	}

	return m_managedCallbacks.m_removeScriptBridge(script) == 1;
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

	// 2) Forward the class name through the managed instance factory
	std::string nativeClassName = owner->GetClassName();
	String      nativeClassNameString(nativeClassName);

	if (m_managedCallbacks.m_createNativeManagedInstance == nullptr)
	{
		return nullptr;
	}

	void* gcHandle = m_managedCallbacks.m_createNativeManagedInstance(&nativeClassNameString, owner);
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

void ScriptSystem::ReleaseGCHandle(void* gcHandle)
{
	if (m_isInitialized && m_managedCallbacks.m_releaseGCHandle != nullptr)
	{
		m_managedCallbacks.m_releaseGCHandle(gcHandle);
	}
}

bool ScriptSystem::Call(void* gcHandle, std::string const& methodName, std::vector<Variant> const& args, Variant& ret)
{
	if (!m_isInitialized || m_managedCallbacks.m_call == nullptr)
	{
		return false;
	}

	String const nativeMethodName(methodName);

	// Convert the std::vector<Variant> to a Variant** array
	std::vector<Variant*> argPointers;
	argPointers.reserve(args.size());
	for (const auto& arg : args)
	{
		argPointers.push_back(const_cast<Variant*>(&arg));
	}

	return m_managedCallbacks
				   .m_call(gcHandle, &nativeMethodName, argPointers.data(), static_cast<int32_t>(args.size()), &ret)
			   ? true
			   : false;
}

void ScriptSystem::RegisterScriptOwner(ObjectID id) { m_scriptOwners.insert(id); }

void ScriptSystem::UnregisterScriptOwner(ObjectID id) { m_scriptOwners.erase(id); }

// Collect the owners and scripts that must be recreated after the assembly reload.
// e.g. CollectReloadInstances() runs before any script instance is released.
void ScriptSystem::CollectReloadInstances()
{
	std::vector<StateBackup> backups;
	auto const               owners = m_scriptOwners;
	for (ObjectID id : owners)
	{
		Object* owner    = ObjectDatabase::GetInstance(id);
		auto*   instance = owner ? dynamic_cast<CSharpInstance*>(owner->GetScriptInstance()) : nullptr;
		if (instance == nullptr)
		{
			continue;
		}

		// Keep the script resource alive across the assembly unload
		backups.push_back(
			StateBackup{ id, Ref<CSharpScript>(static_cast<CSharpScript*>(instance->GetScript().Get())) });
	}
	m_pendingReloadBackups = std::move(backups);
}

void ScriptSystem::DetachScriptInstances()
{
	for (auto const& entry : m_pendingReloadBackups)
	{
		Object* owner = ObjectDatabase::GetInstance(entry.m_owner);
		if (owner != nullptr && owner->GetScriptInstance() != nullptr
			&& owner->GetScriptInstance()->GetScript().Get() == entry.m_script.Get())
		{
			owner->SetScriptInstance(nullptr);
		}
	}
}

// Recreate the script instances that were recorded before the assembly reload.
// e.g. RestoreScriptInstances() runs after the new assembly has been loaded.
bool ScriptSystem::RestoreScriptInstances()
{
	for (auto const& entry : m_pendingReloadBackups)
	{
		Object* owner = ObjectDatabase::GetInstance(entry.m_owner);
		if (owner == nullptr || owner->GetScriptInstance() != nullptr)
		{
			continue;
		}
		m_isRecreatingInstances = true;
		bool const created      = entry.m_script->Instantiate(owner);
		m_isRecreatingInstances = false;
		ERR_FAIL_COND_V_MSG(!created, false, "Failed to recreate script; the recorded instances are retained.\n");
	}
	return true;
}

bool ScriptSystem::ReloadProjectAssembly()
{
	ERR_FAIL_COND_V_MSG(m_isAssemblyReloading || !m_isInitialized, false, "Cannot start script reload.\n");
	m_isAssemblyReloading      = true;
	bool const wasSuspended    = m_scriptExecutionSuspended;
	m_scriptExecutionSuspended = true;
	bool success               = false;
	try
	{
		// 1) Keep the first list across failed reload attempts
		if (!wasSuspended)
		{
			CollectReloadInstances();
		}

		// 2) Release old handles before unloading and replace partial retries
		DetachScriptInstances();
		if (UnloadProjectAssembly() && LoadProjectAssembly() && RestoreScriptInstances())
		{
			m_pendingReloadBackups.clear();
			m_scriptExecutionSuspended = false;
			success                    = true;
		}
	}
	catch (std::exception const& exception)
	{
		ERR_PRINT(Stringf("Script reload failed: %s\n", exception.what()));
	}
	m_isRecreatingInstances = false;
	m_isAssemblyReloading   = false;
	if (success)
	{
		INFO_PRINT("Project assembly reloaded and script instances recreated.\n");
	}
	return success;
}

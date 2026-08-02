#include "MingEngine/Engine/Script/DotNetHost.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include "ThirdParty/DotNetHost/nethost.h"
#include "ThirdParty/DotNetHost/hostfxr.h"

#include <filesystem>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma comment(lib, "ThirdParty/DotNetHost/nethost.lib")

namespace
{
	std::filesystem::path GetExecutableDirectory()
	{
		std::wstring executablePath(32768, L'\0');

		DWORD const length = GetModuleFileNameW(
			nullptr,
			executablePath.data(),
			static_cast<DWORD>(executablePath.size()));

		if (length == 0 || length >= executablePath.size())
		{
			return {};
		}

		executablePath.resize(length);
		return std::filesystem::path(executablePath).parent_path();
	}

	int32_t CORECLR_DELEGATE_CALLTYPE LogUtf8(uint8_t const *text, int32_t textLength)
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
			reinterpret_cast<char const *>(text),
			static_cast<size_t>(textLength));

		DebuggerPrintf(
			"[Managed] %s\n",
			message.c_str());

		return 0;
	}
}

bool DotNetHost::Initialize()
{
	std::filesystem::path const executableDirectory = GetExecutableDirectory();

	std::filesystem::path const managedApiDirectory = executableDirectory / L"MingSharp" / L"Api";
	std::filesystem::path const runtimeConfigPath = managedApiDirectory / L"MingPlugins.runtimeconfig.json";
	std::filesystem::path const managedAssemblyPath = managedApiDirectory / L"MingPlugins.dll";

	if (!std::filesystem::exists(runtimeConfigPath))
	{
		DebuggerPrintf(
			"Missing .NET runtime config: %ls\n",
			runtimeConfigPath.c_str());
		return false;
	}

	if (!std::filesystem::exists(managedAssemblyPath))
	{
		DebuggerPrintf(
			"Missing managed assembly: %ls\n",
			managedAssemblyPath.c_str());
		return false;
	}

	size_t hostfxrPathSize = 0;

	int result = get_hostfxr_path(
		nullptr,
		&hostfxrPathSize,
		nullptr);

	if (hostfxrPathSize == 0)
	{
		DebuggerPrintf(
			"Failed to determine hostfxr path size: 0x%08X\n",
			result);
		return false;
	}

	std::vector<wchar_t> hostfxrPath(hostfxrPathSize);

	result = get_hostfxr_path(
		hostfxrPath.data(),
		&hostfxrPathSize,
		nullptr);

	if (result != 0)
	{
		DebuggerPrintf(
			"Failed to locate hostfxr: 0x%08X\n",
			result);
		return false;
	}

	m_hostfxrModule = LoadLibraryW(hostfxrPath.data());

	if (m_hostfxrModule == nullptr)
	{
		DebuggerPrintf(
			"Failed to load hostfxr: %ls\n",
			hostfxrPath.data());
		return false;
	}

	auto initializeForRuntimeConfig =
		reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
			GetProcAddress(
				static_cast<HMODULE>(m_hostfxrModule),
				"hostfxr_initialize_for_runtime_config"));

	auto getRuntimeDelegate =
		reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
			GetProcAddress(
				static_cast<HMODULE>(m_hostfxrModule),
				"hostfxr_get_runtime_delegate"));

	auto closeHostContext =
		reinterpret_cast<hostfxr_close_fn>(
			GetProcAddress(
				static_cast<HMODULE>(m_hostfxrModule),
				"hostfxr_close"));

	if (initializeForRuntimeConfig == nullptr ||
		getRuntimeDelegate == nullptr ||
		closeHostContext == nullptr)
	{
		DebuggerPrintf("Failed to load required hostfxr exports.\n");
		return false;
	}

	hostfxr_handle hostContext = nullptr;

	result = initializeForRuntimeConfig(
		runtimeConfigPath.c_str(),
		nullptr,
		&hostContext);

	if (result != 0 || hostContext == nullptr)
	{
		DebuggerPrintf(
			"Failed to initialize .NET host context: 0x%08X\n",
			result);

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
		reinterpret_cast<void **>(&loadAssembly));

	closeHostContext(hostContext);
	hostContext = nullptr;

	if (result != 0 || loadAssembly == nullptr)
	{
		DebuggerPrintf(
			"Failed to get .NET assembly loader: 0x%08X\n",
			result);
		return false;
	}

	wchar_t const *typeName = L"MingPlugins.Main, MingPlugins";

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"Initialize",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void **>(&m_initialize));

	if (result != 0 || m_initialize == nullptr)
	{
		DebuggerPrintf(
			"Failed to load managed Initialize: 0x%08X\n",
			result);
		return false;
	}

	result = loadAssembly(
		managedAssemblyPath.c_str(),
		typeName,
		L"Shutdown",
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void **>(&m_shutdown));

	if (result != 0 || m_shutdown == nullptr)
	{
		DebuggerPrintf(
			"Failed to load managed Shutdown: 0x%08X\n",
			result);
		return false;
	}

	NativeCallbacks const nativeCallbacks{&LogUtf8};

	int32_t const initResult = m_initialize(&nativeCallbacks, static_cast<int32_t>(sizeof(nativeCallbacks)));
	if (initResult != 0)
	{
		DebuggerPrintf(
			"Managed Initialize failed: 0x%08X\n",
			initResult);
		return false;
	}

	m_isInitialized = true;

	DebuggerPrintf(".NET Runtime initialized.\n");
	return true;
}

void DotNetHost::Shutdown()
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

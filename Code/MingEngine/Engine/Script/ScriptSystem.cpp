#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

#if defined(_DEBUG)
#pragma comment(lib, "ThirdParty/angelscript/lib/angelscript64d.lib")
#else
#pragma comment(lib, "ThirdParty/angelscript/lib/angelscript64.lib")
#endif

namespace
{
void ScriptMessageCallback(asSMessageInfo const* message, void*)
{
	char const* type = "Info";

	if (message->type == asMSGTYPE_WARNING)
		type = "Warning";
	else if (message->type == asMSGTYPE_ERROR)
		type = "Error";

	DebuggerPrintf("%s (%d, %d): %s: %s\n", message->section, message->row, message->col, type, message->message);
}
} // namespace

ScriptSystem::ScriptSystem([[maybe_unused]] ScriptSystemConfig const& config) {}

void ScriptSystem::Startup()
{
	m_scriptEngine = asCreateScriptEngine();

	GUARANTEE_OR_DIE(m_scriptEngine != nullptr, "Failed to create AngelScript engine.");

	int result = m_scriptEngine->SetMessageCallback(asFUNCTION(ScriptMessageCallback), nullptr, asCALL_CDECL);

	GUARANTEE_OR_DIE(result >= 0, "Failed to register AngelScript message callback.");
}

void ScriptSystem::Shutdown()
{
	m_loadedScripts.clear();

	if (m_scriptEngine != nullptr)
	{
		m_scriptEngine->ShutDownAndRelease();
		m_scriptEngine = nullptr;
	}
}

void ScriptSystem::BeginFrame() {}

void ScriptSystem::EndFrame() {}

std::unique_ptr<ScriptInstance> ScriptSystem::CreateInstance(std::string const& path, Object& owner)
{
	ScriptModule* scriptModule = GetOrCreateModule(path);
	if (scriptModule == nullptr)
	{
		DebuggerPrintf("Failed to load script module for path: %s\n", path.c_str());
		return std::unique_ptr<ScriptInstance>();
	}

	std::unique_ptr<ScriptInstance> instance = ScriptInstance::Create(*scriptModule, owner);
	if (instance == nullptr)
	{
		return nullptr;
	}

	return instance;
}

ScriptModule* ScriptSystem::GetOrCreateModule(std::string const& path)
{
	VirtualPath virtualPath;
	if (!virtualPath.Parse(std::string(path)))
	{
		return nullptr;
	}

	return GetOrCreateModule(virtualPath);
}

ScriptModule* ScriptSystem::GetOrCreateModule(VirtualPath const& virtualPath)
{
	auto it = m_loadedScripts.find(virtualPath);
	if (it != m_loadedScripts.end())
	{
		return &it->second;
	}

	ScriptResourceIdentity identity;

	if (!ScriptResourceIdentity::Create(virtualPath, identity))
	{
		DebuggerPrintf("Invalid script path: %s\n", virtualPath.ToString().c_str());
		return nullptr;
	}

	std::string scriptText;
	if (!g_engine->m_fileSystem->ReadText(virtualPath, scriptText))
	{
		DebuggerPrintf("Failed to read script file: %s\n", virtualPath.ToString().c_str());
		return nullptr;
	}

	// asIScriptModule is an empty handle managed by the AngelScript engine
	// When we call m_scriptEngine->ShutDownAndRelease(); all asIScriptModule instances will be invalidated, so we don't
	// need to worry about cleaning them up individually
	std::string      moduleName   = virtualPath.ToString();
	asIScriptModule* scriptModule = m_scriptEngine->GetModule(moduleName.c_str(), asGM_ALWAYS_CREATE);

	if (scriptModule == nullptr)
	{
		DebuggerPrintf("Failed to create script module for: %s\n", virtualPath.ToString().c_str());
		return nullptr;
	}

	int result = scriptModule->AddScriptSection(virtualPath.ToString().c_str(), scriptText.c_str(), scriptText.size());

	if (result < 0)
	{
		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	result = scriptModule->Build();
	if (result < 0)
	{
		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	asITypeInfo* scriptType = scriptModule->GetTypeInfoByName(identity.GetClassName().c_str());

	if (scriptType == nullptr)
	{
		DebuggerPrintf(
			"Script '%s' does not define class '%s'.\n",
			virtualPath.ToString().c_str(),
			identity.GetClassName().c_str());

		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	ScriptModule module(std::move(identity), scriptModule, scriptType);

	m_loadedScripts[virtualPath] = std::move(module);

	return &m_loadedScripts[virtualPath];
}

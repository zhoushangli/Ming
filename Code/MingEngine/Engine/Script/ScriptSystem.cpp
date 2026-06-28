#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Script/ScriptBindings.hpp"
#include "MingEngine/Engine/Script/ScriptGenerator.hpp"

#include "ThirdParty/angelscript/add_on/scriptstdstring/scriptstdstring.h"
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

void ScriptSystem::BindMethods() {}

void ScriptSystem::Startup()
{
	m_scriptEngine = asCreateScriptEngine();
	GUARANTEE_OR_DIE(m_scriptEngine != nullptr, "Failed to create AngelScript engine.");

	int result = m_scriptEngine->SetMessageCallback(asFUNCTION(ScriptMessageCallback), nullptr, asCALL_CDECL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register AngelScript message callback.");

	// Angel Script Add on
	RegisterStdString(m_scriptEngine);
	RegisterVec3(m_scriptEngine);
	RegisterEulerAngles(m_scriptEngine);
	RegisterMatrix4x4(m_scriptEngine);
	RegisterVariant(m_scriptEngine);

	RegisterNativeObjectType(m_scriptEngine);
	RegisterBridgeFunctions(m_scriptEngine);
	GenerateBuiltinScript(m_scriptEngine);
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

ScriptInstance* ScriptSystem::CreateInstance(Ref<Script> const& script, Object& owner)
{
	ScriptModule* scriptModule = GetOrCreateModule(script->GetVirtualPath());
	if (scriptModule == nullptr)
	{
		DebuggerPrintf("Failed to load script module for path: %s\n", script->GetVirtualPath().c_str());
		return nullptr;
	}

	ScriptInstance* instance = ScriptInstance::Create(*scriptModule, owner);
	if (instance == nullptr)
	{
		return nullptr;
	}
	instance->m_script = script;

	return instance;
}

ScriptModule* ScriptSystem::GetOrCreateModule(std::string const& path)
{
	if (!FileSystem::IsVirtualPath(path))
	{
		return nullptr;
	}

	auto it = m_loadedScripts.find(path);
	if (it != m_loadedScripts.end())
	{
		return &it->second;
	}

	ScriptResourceIdentity identity;

	if (!ScriptResourceIdentity::Create(path, identity))
	{
		DebuggerPrintf("Invalid script path: %s\n", path.c_str());
		return nullptr;
	}

	std::string scriptText;
	if (!g_engine->m_fileSystem->ReadText(path, scriptText))
	{
		DebuggerPrintf("Failed to read script file: %s\n", path.c_str());
		return nullptr;
	}

	// asIScriptModule is an empty handle managed by the AngelScript engine
	// When we call m_scriptEngine->ShutDownAndRelease(); all asIScriptModule instances will be invalidated, so we don't
	// need to worry about cleaning them up individually
	std::string      moduleName   = path;
	asIScriptModule* scriptModule = m_scriptEngine->GetModule(moduleName.c_str(), asGM_ALWAYS_CREATE);

	if (scriptModule == nullptr)
	{
		DebuggerPrintf("Failed to create script module for: %s\n", path.c_str());
		return nullptr;
	}

	// Add wrapper script to the module
	std::string wrapperText;
	if (!g_engine->m_fileSystem->ReadText(kGeneratedScriptResourcePath, wrapperText))
	{
		DebuggerPrintf("Failed to read script file: %s\n", kGeneratedScriptResourcePath);
		return nullptr;
	}

	int result = scriptModule->AddScriptSection("MingEngine.generated.as", wrapperText.c_str(), wrapperText.size());

	if (result < 0)
	{
		DebuggerPrintf("Failed to add MingEngine wrapper script.\n");
		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	// Add the actual script to the module
	result = scriptModule->AddScriptSection(path.c_str(), scriptText.c_str(), scriptText.size());

	if (result < 0)
	{
		DebuggerPrintf("Failed to add script section for: %s\n", path.c_str());
		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	result = scriptModule->Build();
	if (result < 0)
	{
		DebuggerPrintf("Failed to build script module for: %s\n", path.c_str());
		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	asITypeInfo* scriptType = scriptModule->GetTypeInfoByName(identity.GetClassName().c_str());

	if (scriptType == nullptr)
	{
		DebuggerPrintf(
			"Script '%s' does not define class '%s'.\n",
			path.c_str(),
			identity.GetClassName().c_str());

		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	ScriptModule module(std::move(identity), scriptModule, scriptType);

	m_loadedScripts[path] = std::move(module);

	return &m_loadedScripts[path];
}

#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Engine/Script/RegisterBuildinType.hpp"
#include "MingEngine/Engine/Script/ScriptBinder.hpp"
#include "MingEngine/Engine/Script/ScriptGenerator.hpp"

#include "ThirdParty/angelscript/add_on/scriptarray/scriptarray.h"
#include "ThirdParty/angelscript/add_on/scriptbuilder/scriptbuilder.h"
#include "ThirdParty/angelscript/add_on/scriptdictionary/scriptdictionary.h"
#include "ThirdParty/angelscript/add_on/scriptstdstring/scriptstdstring.h"
#include "ThirdParty/angelscript/include/angelscript.h"

#include <memory>

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

void SearchAndRegisterScript(FileEntry const* entry, CScriptBuilder& builder)
{
	if (entry->IsDirectory())
	{
		for (auto const& child : entry->GetChildren())
		{
			SearchAndRegisterScript(child.get(), builder);
		}
	}
	else
	{
		std::string virtualPath = entry->GetVirtualPath();
		if (virtualPath.size() >= 3 && virtualPath.compare(virtualPath.size() - 3, 3, ".as") == 0)
		{
			std::string scriptText;
			if (!g_engine->m_fileSystem->ReadText(virtualPath, scriptText))
			{
				DebuggerPrintf("Failed to read script file: %s\n", virtualPath.c_str());
				return;
			}

			int result =
				builder.AddSectionFromMemory(virtualPath.c_str(), scriptText.c_str(), (unsigned int)scriptText.size());
			if (result < 0)
			{
				DebuggerPrintf("Failed to add script section: %s\n", virtualPath.c_str());
				return;
			}
		}
	}
}

// Our include is more for IDE purposes, we do not need to include any script files
// because we will scan the file system and add all scripts to the module
static int IgnoreScriptInclude(char const* include, char const* from, CScriptBuilder* builder, void* userParam)
{
	include;   // Unused parameter
	from;      // Unused parameter
	builder;   // Unused parameter
	userParam; // Unused parameter
	return 0;
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

	result = m_scriptEngine->SetEngineProperty(asEP_PROPERTY_ACCESSOR_MODE, 2);
	GUARANTEE_OR_DIE(result >= 0, "Failed to set AngelScript engine property.");

	// Angel Script Add on
	RegisterStdString(m_scriptEngine);
	RegisterVec2(m_scriptEngine);
	RegisterVec3(m_scriptEngine);
	RegisterVec4(m_scriptEngine);
	RegisterAABB2(m_scriptEngine);
	RegisterOBB2(m_scriptEngine);
	RegisterCapsule3(m_scriptEngine);
	RegisterEulerAngles(m_scriptEngine);
	RegisterMatrix4x4(m_scriptEngine);
	RegisterVariant(m_scriptEngine);

	RegisterScriptArray(m_scriptEngine, true);
	RegisterScriptDictionary(m_scriptEngine);

	RegisterNativeObjectType(m_scriptEngine);
	RegisterBridgeFunctions(m_scriptEngine);

	GenerateBuiltinScript(m_scriptEngine);

	if (!BuildGameScriptModule())
	{
		ERROR_AND_DIE("Failed to build game script module.");
	}
}

void ScriptSystem::Shutdown()
{
	m_loadedScriptInfos.clear();

	if (m_scriptEngine != nullptr)
	{
		m_scriptEngine->ShutDownAndRelease();
		m_scriptEngine = nullptr;
	}
}

void ScriptSystem::BeginFrame() {}

void ScriptSystem::EndFrame() {}

std::unique_ptr<ScriptInstance> ScriptSystem::CreateInstance(Ref<Script> const& script, Object& owner)
{
	ScriptInstance* instance   = new ScriptInstance();
	asITypeInfo*    scriptType = m_loadedScriptInfos[script->GetName()];
	void*           object     = m_scriptEngine->CreateScriptObject(scriptType);

	instance->m_script = script;
	instance->m_owner  = &owner;
	instance->m_module = m_scriptModule;
	instance->m_object = static_cast<asIScriptObject*>(object);

	asUINT propertyCount = instance->m_object->GetPropertyCount();
	for (asUINT propertyIndex = 0; propertyIndex < propertyCount; ++propertyIndex)
	{
		char const* propertyName = instance->m_object->GetPropertyName(propertyIndex);
		if (propertyName != nullptr && strcmp(propertyName, "nativePtr") == 0)
		{
			void* ownerPropertyAddress                   = instance->m_object->GetAddressOfProperty(propertyIndex);
			*static_cast<Object**>(ownerPropertyAddress) = &owner;
			break;
		}
	}

	instance->m_enterTreeFunction = scriptType->GetMethodByDecl("void _EnterTree()");
	instance->m_exitTreeFunction  = scriptType->GetMethodByDecl("void _ExitTree()");
	instance->m_readyFunction     = scriptType->GetMethodByDecl("void _Ready()");
	instance->m_processFunction   = scriptType->GetMethodByDecl("void _Process(float)");

	return std::unique_ptr<ScriptInstance>(instance);
}

bool ScriptSystem::BuildGameScriptModule()
{
	CScriptBuilder builder;
	builder.SetIncludeCallback(IgnoreScriptInclude, nullptr);

	int result = builder.StartNewModule(m_scriptEngine, "Game");
	if (result < 0)
	{
		DebuggerPrintf("Failed to start game script module");
		return false;
	}

	// 1) Add MingEngine.generated.as to the module
	std::string wrapperText;
	if (!g_engine->m_fileSystem->ReadText(kGeneratedScriptResourcePath, wrapperText))
	{
		DebuggerPrintf("Failed to read script file: %s\n", kGeneratedScriptResourcePath);
		m_scriptEngine->DiscardModule("Game");
		return false;
	}

	result =
		builder.AddSectionFromMemory("MingEngine.generated.as", wrapperText.c_str(), (unsigned int)wrapperText.size());
	if (result < 0)
	{
		DebuggerPrintf("Failed to add MingEngine wrapper script.\n");
		m_scriptEngine->DiscardModule("Game");
		return false;
	}

	// 2) Loop through file system and add all scripts to the module
	FileEntry const* rootEntry = g_engine->m_fileSystem->GetResourceRootEntry();
	if (rootEntry == nullptr)
	{
		DebuggerPrintf("Failed to get resource root entry.\n");
		m_scriptEngine->DiscardModule("Game");
		return false;
	}

	SearchAndRegisterScript(rootEntry, builder);

	result = builder.BuildModule();
	if (result < 0)
	{
		DebuggerPrintf("Failed to build game script module");
		m_scriptEngine->DiscardModule("Game");
		return false;
	}

	m_scriptModule = builder.GetModule();

	m_loadedScriptInfos.clear();

	asUINT typeCount = m_scriptModule->GetObjectTypeCount();
	for (asUINT i = 0; i < typeCount; ++i)
	{
		asITypeInfo* typeInfo = m_scriptModule->GetObjectTypeByIndex(i);
		if (typeInfo == nullptr)
			continue;

		char const* name = typeInfo->GetName();

		m_loadedScriptInfos[name != nullptr ? name : ""] = typeInfo;
	}

	return true;
}
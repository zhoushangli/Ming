#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
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

// When angel script calls a method, it will call this bridge function
// e.g. ScriptNode.SetPosition() --> ScriptMethodBridge() --> actual C++ method
void ScriptMethodBridge(asIScriptGeneric* gen)
{
	MethodInfo const* binding  = static_cast<MethodInfo const*>(gen->GetAuxiliary());
	Object*           object   = static_cast<Object*>(gen->GetObject());
	int               argCount = gen->GetArgCount();
	if (argCount != static_cast<int>(binding->m_argumentTypes.size()))
	{
		return;
	}

	std::vector<Variant> arguments;
	for (int i = 0; i < argCount; ++i)
	{
		switch (binding->m_argumentTypes[i])
		{
		case Variant::Type::Bool:
			arguments.emplace_back(static_cast<bool>(gen->GetArgByte(i) != 0));
			break;
		case Variant::Type::Int:
			arguments.emplace_back(static_cast<int>(gen->GetArgDWord(i)));
			break;
		case Variant::Type::Float:
			arguments.emplace_back(gen->GetArgFloat(i));
			break;
		case Variant::Type::String:
			arguments.emplace_back(*static_cast<std::string*>(gen->GetArgAddress(i)));
			break;
		default:
			DebuggerPrintf("Unsupported argument type for method '%s'.\n", binding->m_name.c_str());
			return;
		}
	}

	Variant result = binding->m_bind->Invoke(*object, arguments);
	switch (binding->m_returnType)
	{
	case Variant::Type::Empty:
		return;
	case Variant::Type::Bool:
		gen->SetReturnByte(result.As<bool>() ? 1 : 0);
		return;
	case Variant::Type::Int:
		gen->SetReturnDWord(static_cast<asDWORD>(result.As<int>()));
		return;
	case Variant::Type::Float:
		gen->SetReturnFloat(result.As<float>());
		return;
	case Variant::Type::String:
		gen->SetReturnObject((void*)&result.As<std::string>());
		return;
	default:
		return;
	}
}

// Transform c++ type to angel script type
// e.g. int -> "int", float -> "float", std::string -> "string"
std::string GetScriptTypeName(Variant::Type type)
{
	// TODO: Add Vec3, EulerAngles, Matrix4x4 support
	switch (type)
	{
	case Variant::Type::Empty:
		return "void";
	case Variant::Type::Bool:
		return "bool";
	case Variant::Type::Int:
		return "int";
	case Variant::Type::Float:
		return "float";
	case Variant::Type::String:
		return "string";
	default:
		return "unknown";
	}
}

std::string BuildMethodDeclaration(MethodInfo const& methodInfo)
{
	std::string declaration;

	declaration += GetScriptTypeName(methodInfo.m_returnType);
	declaration += " " + methodInfo.m_name + "(";

	for (size_t i = 0; i < methodInfo.m_argumentTypes.size(); ++i)
	{
		declaration += GetScriptTypeName(methodInfo.m_argumentTypes[i]);
		if (i < methodInfo.m_argumentTypes.size() - 1)
		{
			declaration += ", ";
		}
	}

	declaration += ")";

	if (methodInfo.m_isConst)
	{
		declaration += " const";
	}

	return declaration;
}

} // namespace

ScriptSystem::ScriptSystem([[maybe_unused]] ScriptSystemConfig const& config) {}

void ScriptSystem::Startup()
{
	m_scriptEngine = asCreateScriptEngine();
	GUARANTEE_OR_DIE(m_scriptEngine != nullptr, "Failed to create AngelScript engine.");

	int result = m_scriptEngine->SetMessageCallback(asFUNCTION(ScriptMessageCallback), nullptr, asCALL_CDECL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register AngelScript message callback.");

	// Register class database to script
	for (ClassInfo const* classInfo : ClassDatabase::GetRegisteredClasses())
	{
		result = m_scriptEngine->RegisterObjectType(classInfo->m_className.c_str(), 0, asOBJ_REF | asOBJ_NOCOUNT);
		GUARANTEE_OR_DIE(result >= 0, Stringf("Failed to register script class: %s", classInfo->m_className.c_str()));

		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo == nullptr)
			{
				continue;
			}

			bool flag = false;

			if (methodInfo->m_returnType == Variant::Type::Vec3
				|| methodInfo->m_returnType == Variant::Type::EulerAngles
				|| methodInfo->m_returnType == Variant::Type::Matrix4x4
				|| methodInfo->m_returnType == Variant::Type::String)
			{
				flag = true;
			}
			for (Variant::Type argType : methodInfo->m_argumentTypes)
			{
				if (argType == Variant::Type::Empty || argType == Variant::Type::Vec3
					|| argType == Variant::Type::EulerAngles || argType == Variant::Type::Matrix4x4
					|| argType == Variant::Type::String)
				{
					flag = true;
					break;
				}
			}

			if (flag)
			{
				continue;
			}

			std::string methodDeclaration = BuildMethodDeclaration(*methodInfo);
			result                        = m_scriptEngine->RegisterObjectMethod(
				classInfo->m_className.c_str(),
				methodDeclaration.c_str(),
				asFUNCTION(ScriptMethodBridge),
				asCALL_GENERIC,
				methodInfo.get());
			GUARANTEE_OR_DIE(
				result >= 0,
				Stringf(
					"Failed to register method '%s' for script class '%s'.",
					methodInfo->m_name.c_str(),
					classInfo->m_className.c_str()));
		}
	}
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
		DebuggerPrintf("Failed to add script section for: %s\n", virtualPath.ToString().c_str());
		m_scriptEngine->DiscardModule(moduleName.c_str());
		return nullptr;
	}

	result = scriptModule->Build();
	if (result < 0)
	{
		DebuggerPrintf("Failed to build script module for: %s\n", virtualPath.ToString().c_str());
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

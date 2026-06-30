#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Script.hpp"

#include "MingEngine/Engine/Script/ScriptInstance.hpp"

#include <string>
#include <unordered_map>

class asIScriptEngine;
class asIScriptModule;
class asITypeInfo;

struct ScriptSystemConfig
{
	bool m_isEnabled = true;
};

class ScriptSystem : public SystemBase
{
	MCLASS(ScriptSystem, SystemBase)

public:
	ScriptSystem(ScriptSystemConfig const& config);

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	std::unique_ptr<ScriptInstance> CreateInstance(Ref<Script> const& script, Object& owner);

	static void BindMethods();

private:
	bool BuildGameScriptModule();

private:
	// We use virtual path as the key
	// So that the error will be like: Scripts/Player/PlayerController.as (12, 5): Error: ...
	std::unordered_map<std::string, asITypeInfo*> m_loadedScriptInfos;

	asIScriptEngine* m_scriptEngine = nullptr;
	asIScriptModule* m_scriptModule = nullptr;
};

#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Script.hpp"

#include "MingEngine/Engine/Script/ScriptInstance.hpp"
#include "MingEngine/Engine/Script/ScriptModule.hpp"
#include "MingEngine/Engine/Script/ScriptResourceIdentity.hpp"

#include <string>
#include <unordered_map>

class asIScriptEngine;

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
	ScriptModule* GetOrCreateModule(std::string const& virtualPath);

private:
	std::unordered_map<std::string, ScriptModule> m_loadedScripts;

	asIScriptEngine* m_scriptEngine = nullptr;
};


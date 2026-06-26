#pragma once

#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Script.hpp"

#include "MingEngine/Engine/File/VirtualPath.hpp"
#include "MingEngine/Engine/Script/ScriptInstance.hpp"
#include "MingEngine/Engine/Script/ScriptModule.hpp"
#include "MingEngine/Engine/Script/ScriptResourceIdentity.hpp"

#include <unordered_map>

class asIScriptEngine;

struct ScriptSystemConfig
{
	bool m_isEnabled = true;
};

class ScriptSystem
{
public:
	ScriptSystem(ScriptSystemConfig const& config);

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	ScriptInstance* CreateInstance(Ref<Script> const& script, Object& owner);

private:
	ScriptModule* GetOrCreateModule(std::string const& path);
	ScriptModule* GetOrCreateModule(VirtualPath const& virtualPath);

private:
	std::unordered_map<VirtualPath, ScriptModule, VirtualPathHash> m_loadedScripts;

	asIScriptEngine* m_scriptEngine = nullptr;
};


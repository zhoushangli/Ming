#pragma once

#include "MingEngine/Engine/Script/ScriptResourceIdentity.hpp"
#include "MingEngine/Engine/Script/ScriptModule.hpp"

#include <unordered_map>

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

	ScriptModule* LoadScript(VirtualPath const& path);

private:
	std::unordered_map<VirtualPath, ScriptModule> m_loadedScripts;
};
#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

ScriptSystem::ScriptSystem(ScriptSystemConfig const& config) {}

ScriptModule* ScriptSystem::LoadScript(VirtualPath const& path)
{
	auto it = m_loadedScripts.find(path);
	if (it != m_loadedScripts.end())
	{
		return &it->second;
	}

	ScriptResourceIdentity identity(path);

	ScriptModule scriptModule;
	scriptModule.m_identity = identity;

	m_loadedScripts[path] = scriptModule;

	return &m_loadedScripts[path];
}

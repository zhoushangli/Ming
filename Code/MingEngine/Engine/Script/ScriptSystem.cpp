#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

ScriptSystem::ScriptSystem([[maybe_unused]] ScriptSystemConfig const& config) {}

void ScriptSystem::Startup()
{
	bool result = m_dotNetHost.Initialize();
	GUARANTEE_OR_DIE(result, "Failed to initialize .NET Runtime.");
}

void ScriptSystem::Shutdown()
{
	m_dotNetHost.Shutdown();
}

void ScriptSystem::BeginFrame() {}

void ScriptSystem::EndFrame() {}

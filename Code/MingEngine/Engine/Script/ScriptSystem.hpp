#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Script/DotNetHost.hpp"

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

private:
	DotNetHost m_dotNetHost;
};

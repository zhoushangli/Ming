#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

struct ImGuiConfig
{
	bool m_isEnable = true;
};

class ImGuiSystem : public SystemBase
{
	MCLASS(ImGuiSystem, SystemBase)

public:
	ImGuiSystem(ImGuiConfig config);
	~ImGuiSystem();

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	static void BindMethods();

private:
	ImGuiConfig m_config;
};

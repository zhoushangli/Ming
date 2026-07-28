#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

struct ImGuiSystemConfig
{
	bool m_isEnable = true;
};

class ImGuiSystem : public SystemBase
{
	MCLASS(ImGuiSystem, SystemBase)

public:
	ImGuiSystem(ImGuiSystemConfig config);
	~ImGuiSystem();

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	bool WantCaptureMouse() const;
	bool WantCaptureKeyboard() const;

	static void BindMethods();

private:
	ImGuiSystemConfig m_config;
};

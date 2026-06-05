#pragma once

struct ImGuiConfig
{
	bool m_isEnable = true;
};

class ImGuiSystem
{
public:
	ImGuiSystem(ImGuiConfig config);
	~ImGuiSystem();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

private:
	ImGuiConfig m_config;
};

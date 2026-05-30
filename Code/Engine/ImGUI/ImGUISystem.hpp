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
	void RenderEditorUI();
	void RenderMainMenuBar();
	void RenderDockSpace();

	void RenderScenePanel();
	void RenderFileSystemPanel();
	void RenderViewportPanel();
	void RenderInspectorPanel();
	void RenderOutputPanel();

private:
	ImGuiConfig m_config;

	bool m_showDemoWindow       = false;
	bool m_showScenePanel       = true;
	bool m_showFileSystemPanel  = true;
	bool m_showViewportPanel    = true;
	bool m_showInspectorPanel   = true;
	bool m_showOutputPanel      = true;
};

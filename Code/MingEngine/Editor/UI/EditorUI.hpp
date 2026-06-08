#pragma once

#include "MingEngine/Editor/UI/CreateNodePanel.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"
#include "MingEngine/Editor/UI/InspectorPanel.hpp"
#include "MingEngine/Editor/UI/OutputPanel.hpp"
#include "MingEngine/Editor/UI/ScenePanel.hpp"
#include "MingEngine/Editor/UI/ViewportPanel.hpp"

struct EditorUIContext;

class EditorUI
{
public:
	void Render(EditorUIContext& context);

private:
	void RenderMainMenuBar();
	void RenderDockSpace();

private:
	ScenePanel      m_scenePanel;
	CreateNodePanel m_createNodePanel;
	FileSystemPanel m_fileSystemPanel;
	ViewportPanel   m_viewportPanel;
	InspectorPanel  m_inspectorPanel;
	OutputPanel     m_outputPanel;
};

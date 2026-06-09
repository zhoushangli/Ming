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

	template<typename TPanel>
	TPanel& GetPanel();

	template<typename TPanel>
	void OpenPanel(typename TPanel::Data const& data)
	{
		GetPanel<TPanel>().Open(data);
	}

	template<typename TPanel>
	void ClosePanel()
	{
		GetPanel<TPanel>().Close();
	}

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

template<>
inline ScenePanel& EditorUI::GetPanel<ScenePanel>()
{
	return m_scenePanel;
}

template<>
inline CreateNodePanel& EditorUI::GetPanel<CreateNodePanel>()
{
	return m_createNodePanel;
}

template<>
inline FileSystemPanel& EditorUI::GetPanel<FileSystemPanel>()
{
	return m_fileSystemPanel;
}

template<>
inline ViewportPanel& EditorUI::GetPanel<ViewportPanel>()
{
	return m_viewportPanel;
}

template<>
inline InspectorPanel& EditorUI::GetPanel<InspectorPanel>()
{
	return m_inspectorPanel;
}

template<>
inline OutputPanel& EditorUI::GetPanel<OutputPanel>()
{
	return m_outputPanel;
}

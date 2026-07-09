#pragma once

#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Editor/UI/CreateNodePanel.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"
#include "MingEngine/Editor/UI/ImportPanel.hpp"
#include "MingEngine/Editor/UI/InspectorPanel.hpp"
#include "MingEngine/Editor/UI/OutputPanel.hpp"
#include "MingEngine/Editor/UI/ScenePanel.hpp"
#include "MingEngine/Editor/UI/ViewportPanel.hpp"

struct EditorUIContext;

class EditorUI
{
private:
	struct WarningData
	{
		std::string m_title;
		std::string m_message;
	};

public:
	void Render(EditorUIContext& context);

	void Warning(std::string const& title, std::string const& message);
	void SetResourceDropAllowed(bool allowed);

	// Viewport rect — set every frame by ViewportPanel, consumed by EditorCamera / GizmoContext
	void SetViewportRect(Vec2 origin, Vec2 dims);
	Vec2 GetViewportOrigin() const;
	Vec2 GetViewportDimensions() const;
	Vec2 ToViewportPos(Vec2 windowPos) const;

	template <typename TPanel>
	TPanel& GetPanel();

	template <typename TPanel>
	void OpenPanel(typename TPanel::Data const& data)
	{
		GetPanel<TPanel>().Open(data);
	}

	template <typename TPanel>
	void ClosePanel()
	{
		GetPanel<TPanel>().Close();
	}

private:
	void RenderMainMenuBar();
	void RenderDockSpace();
	void RenderWarningPopup();
	void BeginResourceDragDropFrame();
	void ApplyResourceDragDropCursor();

private:
	ScenePanel      m_scenePanel;
	CreateNodePanel m_createNodePanel;
	FileSystemPanel m_fileSystemPanel;
	ImportPanel     m_importPanel;
	ViewportPanel   m_viewportPanel;
	InspectorPanel  m_inspectorPanel;
	OutputPanel     m_outputPanel;

	bool        m_showWarningPopup = false;
	WarningData m_warningData;
	bool        m_resourceDropAllowed = false;

	Vec2 m_viewportOrigin = Vec2::Zero;
	Vec2 m_viewportDims   = Vec2::Zero;
};

template <>
inline ScenePanel& EditorUI::GetPanel<ScenePanel>()
{
	return m_scenePanel;
}

template <>
inline CreateNodePanel& EditorUI::GetPanel<CreateNodePanel>()
{
	return m_createNodePanel;
}

template <>
inline FileSystemPanel& EditorUI::GetPanel<FileSystemPanel>()
{
	return m_fileSystemPanel;
}

template <>
inline ImportPanel& EditorUI::GetPanel<ImportPanel>()
{
	return m_importPanel;
}

template <>
inline ViewportPanel& EditorUI::GetPanel<ViewportPanel>()
{
	return m_viewportPanel;
}

template <>
inline InspectorPanel& EditorUI::GetPanel<InspectorPanel>()
{
	return m_inspectorPanel;
}

template <>
inline OutputPanel& EditorUI::GetPanel<OutputPanel>()
{
	return m_outputPanel;
}

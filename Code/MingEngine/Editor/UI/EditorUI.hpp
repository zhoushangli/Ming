#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"
#include "MingEngine/Editor/UI/ImportPanel.hpp"
#include "MingEngine/Editor/UI/InspectorPanel.hpp"
#include "MingEngine/Editor/UI/OutputPanel.hpp"
#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"
#include "MingEngine/Editor/UI/Popup/WarningPopup.hpp"
#include "MingEngine/Editor/UI/ScenePanel.hpp"
#include "MingEngine/Editor/UI/ViewportPanel.hpp"

struct EditorUIContext;

class EditorUI
{
public:
	void Render(EditorUIContext& context);

	void Warning(std::string const& title, std::string const& message);

	// Viewport rect — set every frame by ViewportPanel, consumed by EditorCamera / GizmoContext
	void    SetViewportRect(Vector2 origin, Vector2 dims);
	Vector2 GetViewportOrigin() const;
	Vector2 GetViewportDimensions() const;
	Vector2 ToViewportPos(Vector2 windowPos) const;

	template <typename TPanel>
	TPanel& GetPanel();

private:
	void RenderMainMenuBar();
	void RenderDockSpace();
	void ApplyDragDropCursor();

private:
	ScenePanel      m_scenePanel;
	FileSystemPanel m_fileSystemPanel;
	ImportPanel     m_importPanel;
	ViewportPanel   m_viewportPanel;
	InspectorPanel  m_inspectorPanel;
	OutputPanel     m_outputPanel;

	ProjectSettingsPopup m_projectSettingsPopup;
	WarningPopup         m_warningPopup;

	Vector2 m_viewportOrigin = Vector2::Zero;
	Vector2 m_viewportDims   = Vector2::Zero;
};

template <>
inline ScenePanel& EditorUI::GetPanel<ScenePanel>()
{
	return m_scenePanel;
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

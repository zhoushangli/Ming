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
#include "MingEngine/Scene/Resource/FontResource.hpp"
#include <map>

class EditorNode;
class ColorRect;
class Control;
class Label;
class LineEdit;
class Button;
class Tree;
class TabContainer;
class ScrollContainer;
class SubViewportContainer;
class PopupPanel;

struct EditorUIContext;

class EditorUI
{
public:
	void Render(EditorUIContext& context);
	bool HandleInput(InputEvent const& event);
	bool IsNativeInputActive() const;
	void FinishInput(EditorUIContext& context);
	void InitializeNative(EditorNode* owner);
	void RenderNative(EditorUIContext& context);
	void RenderLegacy(EditorUIContext& context);
	void ShowUnsavedScene();
	bool IsSceneNavigationActive() const;
	bool IsSceneHovered() const;
	SubViewportContainer* GetSceneView() const { return m_sceneView; }
	Tree* GetSceneTreeControl() const { return m_nativeSceneTree; }
	Tree* GetFileTreeControl() const { return m_nativeFileTree; }
	TabContainer* GetTabs() const { return m_tabs; }
	void FinishNativeEditing();

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
	void UpdateSceneItems();
	void UpdateFileItems();
	void ShowNativePopup(int mode, String const& message);
	void AcceptNativePopup(int choice);

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
	EditorNode* m_owner = nullptr;
	ColorRect* m_nativeRoot = nullptr;
	Control* m_toolbar = nullptr;
	Label* m_sceneTitle = nullptr;
	ColorRect* m_inspectorBackground = nullptr;
	Label* m_inspectorTitle = nullptr;
	Label* m_emptyInspector = nullptr;
	TabContainer* m_tabs = nullptr;
	Control* m_scenePage = nullptr;
	Control* m_filePage = nullptr;
	LineEdit* m_sceneFilter = nullptr;
	LineEdit* m_fileFilter = nullptr;
	Tree* m_nativeSceneTree = nullptr;
	Tree* m_nativeFileTree = nullptr;
	ScrollContainer* m_sceneScroll = nullptr;
	ScrollContainer* m_fileScroll = nullptr;
	SubViewportContainer* m_sceneView = nullptr;
	PopupPanel* m_nativePopup = nullptr;
	ColorRect* m_popupPanel = nullptr;
	Label* m_popupMessage = nullptr;
	LineEdit* m_popupPath = nullptr;
	Button* m_popupButtons[3] = {};
	int m_popupMode = 0;
	bool m_navigating = false;
	bool m_filesDirty = true;
	Ref<FontResource> m_nativeFont;
	std::map<uint64_t, ObjectID> m_nodeItems;
	std::map<uint64_t, VirtualPath> m_fileItems;
	std::map<std::string, uint64_t> m_fileKeys;
	uint64_t m_nextFileKey = 1;
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

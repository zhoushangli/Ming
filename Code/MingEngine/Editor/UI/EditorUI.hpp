#pragma once

#include "MingEngine/Editor/UI/FileSystemPanel.hpp"
#include "MingEngine/Editor/UI/ImportPanel.hpp"
#include "MingEngine/Editor/UI/InspectorPanel.hpp"
#include "MingEngine/Editor/UI/OutputPanel.hpp"
#include "MingEngine/Editor/UI/ScenePanel.hpp"
#include "MingEngine/Editor/UI/ViewportPanel.hpp"
#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"
#include "MingEngine/Editor/UI/Popup/WarningPopup.hpp"
#include "MingEngine/Scene/GUI/MenuBar.hpp"

#include <array>

class EditorNode;
class ColorRect;
class TabContainer;
class SplitContainer;
class TextureRect;
class MenuBar;

class EditorUI
{
public:
	using MenuItem = MenuBar::MenuItem;

private:
	struct DockPanel
	{
		String m_id;
		Control* m_root = nullptr;
	};

	struct ToolbarAction
	{
		Button* m_button = nullptr;
		std::string m_icon;
		String m_text;
	};

public:
	~EditorUI();
	void InitializeNative(EditorNode* owner);
	void Render(EditorUIContext& context);
	bool HandleInput(InputEvent const& event);
	bool IsNativeInputActive() const;
	void FinishInput(EditorUIContext& context);
	void FinishNativeEditing();
	void ShowUnsavedScene();
	void Warning(std::string const& title, std::string const& message);
	void ShowMenu(Control* source, Vector2 position, std::vector<MenuItem> const& items);
	void SetNativeDragPreview(Control* source, String const& text);

	Control* GetRootControl() const;
	bool IsSceneNavigationActive() const;
	bool IsSceneHovered() const;
	SubViewportContainer* GetSceneView() const { return m_sceneView; }
	Tree* GetSceneTreeControl() const { return m_scenePanel->GetTree(); }
	Tree* GetFileTreeControl() const { return m_fileSystemPanel->GetTree(); }
	TabContainer* GetTabs() const { return m_tabs; }
	TabContainer* GetDockSlot(int index) const;
	SplitContainer* GetDockSplit(int index) const;
	bool MoveNativePanel(String const& id, int slot, int index = -1);
	void ResetNativeLayout();
	bool SaveNativeLayout();
	void LoadNativeLayout();
	void SetStatusMessage(String const& message);
	void ToggleOutputCollapsed();

	void SetViewportRect(Vector2 origin, Vector2 dims);
	Vector2 GetViewportOrigin() const { return m_viewportOrigin; }
	Vector2 GetViewportDimensions() const { return m_viewportDims; }
	Vector2 ToViewportPos(Vector2 position) const;

	template <typename TPanel>
	TPanel& GetPanel();

private:
	void CreateToolbar();
	void ShowNativePopup(int mode, String const& message);
	void AcceptNativePopup(int choice);
	void InitializeNativeDocking();
	void UpdateNativeDocking();
	int FindNativePanel(String const& id) const;
	int GetNativeDockDestination(int slot, Vector2 const& position, int& index) const;
	void TogglePanel(EditorPanel* panel);

private:
	ScenePanel* m_scenePanel = nullptr;
	FileSystemPanel* m_fileSystemPanel = nullptr;
	ImportPanel* m_importPanel = nullptr;
	ViewportPanel* m_viewportPanel = nullptr;
	InspectorPanel* m_inspectorPanel = nullptr;
	OutputPanel* m_outputPanel = nullptr;
	ProjectSettingsPopup m_projectSettingsPopup;
	WarningPopup m_warningPopup;
	EditorNode* m_owner = nullptr;
	ColorRect* m_nativeRoot = nullptr;
	MenuBar* m_menuBar = nullptr;
	Control* m_actions = nullptr;
	Control* m_workspaces = nullptr;
	TextureRect* m_brand = nullptr;
	TextureRect* m_beam = nullptr;
	Label* m_status = nullptr;
	HBoxContainer* m_statusBar = nullptr;
	Label* m_statusMessage = nullptr;
	Label* m_statusInfo = nullptr;
	std::vector<ToolbarAction> m_toolbarActions;
	TabContainer* m_tabs = nullptr;
	Control* m_scenePage = nullptr;
	Control* m_filePage = nullptr;
	Control* m_inspectorBackground = nullptr;
	SubViewportContainer* m_sceneView = nullptr;
	PopupPanel* m_nativePopup = nullptr;
	VBoxContainer* m_popupContent = nullptr;
	Label* m_popupMessage = nullptr;
	LineEdit* m_popupPath = nullptr;
	Button* m_popupButtons[3] = {};
	int m_popupMode = 0;
	PopupPanel* m_menu = nullptr;
	VBoxContainer* m_menuContent = nullptr;
	Vector2 m_viewportOrigin;
	Vector2 m_viewportDims;
	std::array<DockPanel, 5> m_dockPanels;
	std::array<TabContainer*, 4> m_dockSlots = {};
	std::array<SplitContainer*, 4> m_dockSplits = {};
	std::array<ColorRect*, 4> m_dockHints = {};
	ColorRect* m_dockInsertMark = nullptr;
	bool m_loadingLayout = false;
	bool m_layoutReady = false;
	float m_outputSplitRatio = 0.8f;
};

template <>
inline ScenePanel& EditorUI::GetPanel<ScenePanel>()
{
	return *m_scenePanel;
}

template <>
inline FileSystemPanel& EditorUI::GetPanel<FileSystemPanel>()
{
	return *m_fileSystemPanel;
}

template <>
inline ImportPanel& EditorUI::GetPanel<ImportPanel>()
{
	return *m_importPanel;
}

template <>
inline ViewportPanel& EditorUI::GetPanel<ViewportPanel>()
{
	return *m_viewportPanel;
}

template <>
inline InspectorPanel& EditorUI::GetPanel<InspectorPanel>()
{
	return *m_inspectorPanel;
}

template <>
inline OutputPanel& EditorUI::GetPanel<OutputPanel>()
{
	return *m_outputPanel;
}

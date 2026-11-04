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

#include "MingEngine/Editor/UI/Popup/CreateScriptPopup.hpp"
#include <array>
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/Popup/SceneFilePopup.hpp"

class EditorNode;
class ColorRect;
class TabContainer;
class SplitContainer;
class TextureRect;
class MenuBar;
class OptionButton;

class EditorUI
{
public:
	using MenuItem = MenuBar::MenuItem;

private:
	struct DockPanel
	{
		String   m_id;
		Control* m_root = nullptr;
	};

	struct ToolbarAction
	{
		Button*     m_button = nullptr;
		std::string m_icon;
		String      m_text;
	};

	struct MenuLevel
	{
		Control* m_panel = nullptr;
		Button* m_parent = nullptr;
		std::vector<Button*> m_buttons;
		std::vector<MenuItem> m_items;
	};

public:
	~EditorUI();
	void InitializeNative(EditorNode* owner);
	void Render(EditorUIContext& context);
	bool HandleInput(InputEvent const& event);
	bool IsNativeInputActive() const;
	void FinishInput(EditorUIContext& context);
	void FinishNativeEditing();
	void ShowUnsavedScene(String const& message, String const& question, String const& action);
	void CreateScript(VirtualPath const& directory, std::string const& base, std::function<void(VirtualPath const&)> created);
	void OpenSceneFile(
		SceneFilePopup::Mode                    mode,
		VirtualPath const&                      initial,
		std::function<bool(VirtualPath const&)> accepted,
		std::function<void()>                   cancelled = {});
	void Warning(std::string const& title, std::string const& message);
	void ShowMenu(Control* source, Vector2 position, std::vector<MenuItem> const& items);
	void SetNativeDragPreview(Control* source, String const& text);
	void OpenScriptFile(std::function<void(VirtualPath const&)> selected);
	void HandleMenuPointer(InputEvent const& event);

	Control*              GetRootControl() const;
	bool                  IsSceneNavigationActive() const;
	bool                  IsSceneHovered() const;
	SubViewportContainer* GetSceneView() const { return m_sceneView; }
	Tree*                 GetSceneTreeControl() const { return m_scenePanel->GetTree(); }
	Tree*                 GetFileTreeControl() const { return m_fileSystemPanel->GetTree(); }

	TabContainer*   GetDockSlot(int index) const;
	SplitContainer* GetDockSplit(int index) const;
	bool            MoveNativePanel(String const& id, int slot, int index = -1);
	void            ResetNativeLayout();
	bool            SaveNativeLayout();
	void            LoadNativeLayout();
	void            SetStatusMessage(String const& message);

	void ToggleOutputCollapsed();

	void    SetViewportRect(Vector2 origin, Vector2 dims);
	Vector2 GetViewportOrigin() const { return m_viewportOrigin; }
	Vector2 GetViewportDimensions() const { return m_viewportDims; }
	Vector2 ToViewportPos(Vector2 position) const;

	template <typename TPanel>
	TPanel& GetPanel();

private:
	void                  CreateToolbar();
	void                  CreateMenuBar();
	void                  CreateRunControls();
	void                  CreateStatusBar();
	std::vector<MenuItem> CreateMenuItems(std::string const& menu);
	void                  AcceptUnsavedScene(EditorNode::SceneActionChoice choice);
	void                  ShowEditorSettings();
	void OpenSubmenu(Button* parent, std::vector<MenuItem> const& items);
	void HandleMenuKey(InputEvent const& event);

	void InitializeNativeDocking();
	void UpdateNativeDocking();
	int  FindNativePanel(String const& id) const;
	int  GetNativeDockDestination(int slot, Vector2 const& position, int& index) const;
	void TogglePanel(EditorPanel* panel);

private:
	ScenePanel*                m_scenePanel      = nullptr;
	FileSystemPanel*           m_fileSystemPanel = nullptr;
	ImportPanel*               m_importPanel     = nullptr;
	ViewportPanel*             m_viewportPanel   = nullptr;
	InspectorPanel*            m_inspectorPanel  = nullptr;
	OutputPanel*               m_outputPanel     = nullptr;
	PopupPanel*                m_settingsPopup   = nullptr;
	OptionButton*              m_settingsScale   = nullptr;
	PopupPanel* m_scriptFilePopup = nullptr;
	VBoxContainer* m_scriptFileContent = nullptr;
	CreateScriptPopup          m_createScriptPopup;
	SceneFilePopup             m_sceneFilePopup;
	ProjectSettingsPopup       m_projectSettingsPopup;
	WarningPopup               m_warningPopup;
	EditorNode*                m_owner         = nullptr;
	ColorRect*                 m_nativeRoot    = nullptr;
	MenuBar*                   m_menuBar       = nullptr;
	Control*                   m_actions       = nullptr;
	Button*                    m_playButton    = nullptr;
	Button*                    m_stopButton    = nullptr;
	Control*                   m_workspaces    = nullptr;
	TextureRect*               m_brand         = nullptr;
	TextureRect*               m_beam          = nullptr;
	Label*                     m_status        = nullptr;
	HBoxContainer*             m_statusBar     = nullptr;
	Label*                     m_statusMessage = nullptr;
	Label*                     m_statusInfo    = nullptr;
	std::vector<ToolbarAction> m_toolbarActions;

	SubViewportContainer* m_sceneView    = nullptr;
	PopupPanel*           m_unsavedPopup = nullptr;
	Label* m_unsavedMessage = nullptr;
	Label* m_unsavedQuestion = nullptr;
	Button* m_unsavedSave = nullptr;
	HBoxContainer* m_unsavedActions = nullptr;

	PopupPanel*                    m_menu        = nullptr;
	VBoxContainer*                 m_menuContent = nullptr;
	std::vector<MenuLevel> m_menuLevels;
	Vector2                        m_viewportOrigin;
	Vector2                        m_viewportDims;
	std::array<DockPanel, 5>       m_dockPanels;
	std::array<TabContainer*, 4>   m_dockSlots             = {};
	std::array<SplitContainer*, 4> m_dockSplits            = {};
	std::array<ColorRect*, 4>      m_dockHints             = {};
	ColorRect*                     m_dockInsertMark        = nullptr;
	bool                           m_resolvingUnsavedPopup = false;
	bool                           m_loadingLayout         = false;
	bool                           m_layoutReady           = false;
	float                          m_outputSplitRatio      = 0.8f;
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

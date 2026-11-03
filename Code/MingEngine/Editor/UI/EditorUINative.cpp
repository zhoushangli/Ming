#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/GUI/MenuBar.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"

#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include <algorithm>
#include <cmath>
#include "ThirdParty/GLFW/glfw3.h"

namespace
{
void Place(Control* control, float x, float y, float width, float height)
{
	control->SetPosition(Vector2(x, y));
	control->UpdateSize(Vector2(std::max(1.f, width), std::max(1.f, height)));
}
}

void EditorUI::InitializeNative(EditorNode* owner)
{
	if (m_nativeRoot != nullptr)
	{
		return;
	}

	m_owner = owner;
	ClassDatabase::RegisterAbstractClass<EditorPanel>();
	ClassDatabase::RegisterClass<ScenePanel>();
	ClassDatabase::RegisterClass<FileSystemPanel>();
	ClassDatabase::RegisterClass<InspectorPanel>();
	ClassDatabase::RegisterClass<ImportPanel>();
	ClassDatabase::RegisterClass<ViewportPanel>();
	ClassDatabase::RegisterClass<OutputPanel>();
	m_nativeRoot = MemNew<ColorRect>();
	m_nativeRoot->SetName("EditorUI");
	m_nativeRoot->SetSerializable(false);
	m_nativeRoot->SetClipContents(true);
	owner->AddNode(m_nativeRoot);
	ThemeDatabase::SetThemeContext(m_nativeRoot, { owner->GetEditorTheme() });
	m_scenePanel      = MemNew<ScenePanel>();
	m_fileSystemPanel = MemNew<FileSystemPanel>();
	m_inspectorPanel  = MemNew<InspectorPanel>();
	m_importPanel     = MemNew<ImportPanel>();
	m_outputPanel     = MemNew<OutputPanel>();
	m_viewportPanel   = MemNew<ViewportPanel>();
	EditorUIContext context;
	context.m_editorUI    = this;
	context.m_rootControl = m_nativeRoot;
	context.m_selection   = &owner->GetSelection();
	context.m_sceneTree   = owner->GetSceneTree();
	context.m_fileSystem  = g_engine->m_fileSystem;
	for (EditorPanel* panel : std::initializer_list<EditorPanel*>{ m_scenePanel,
																   m_fileSystemPanel,
																   m_inspectorPanel,
																   m_importPanel,
																   m_outputPanel,
																   m_viewportPanel })
	{
		m_nativeRoot->AddNode(panel);
		panel->Initialize(context);
	}

	m_sceneView = m_viewportPanel->GetSceneView();
	ThemeDatabase::SetThemeContext(m_sceneView->GetSubViewport(), {});
	owner->GetSceneTree()->SetSceneViewport(m_sceneView->GetSubViewport());
	CreateToolbar();
	g_engine->m_fileSystem->ScanResourceTree();
	InitializeNativeDocking();
}

void EditorUI::CreateToolbar()
{
	CreateMenuBar();
	CreateRunControls();
	CreateStatusBar();
}

void EditorUI::CreateMenuBar()
{
	m_beam = MemNew<TextureRect>();
	m_beam->SetTexture(EditorIcons::GetBrandTexture(true));
	m_beam->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	m_nativeRoot->AddNode(m_beam);
	m_brand = MemNew<TextureRect>();
	m_brand->SetTexture(EditorIcons::GetBrandTexture(false));
	m_brand->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	m_nativeRoot->AddNode(m_brand);
	m_menuBar = MemNew<MenuBar>();
	m_nativeRoot->AddNode(m_menuBar);
	m_menuBar->SetAboutToOpenCallback(
		[this]()
		{
			FinishNativeEditing();
		});

	for (char const* name : { "Scene", "Project", "Editor", "View", "Help" })
	{
		m_menuBar->AddMenu(
			name,
			[this, menu = std::string(name)]()
			{
				return CreateMenuItems(menu);
			});
	}
}

std::vector<EditorUI::MenuItem> EditorUI::CreateMenuItems(std::string const& menu)
{
	if (menu == "Scene")
	{
		bool const hasScene = m_owner->HasScene();
		return { { "New Scene",
				   [this]()
				   {
					   m_owner->RequestNewScene();
				   },
				   "Ctrl+N" },
				 { "Open Scene...",
				   [this]()
				   {
					   m_owner->RequestOpenScene();
				   },
				   "Ctrl+O" },
				 { "", {}, "", true },
				 { "Save Scene",
				   [this]()
				   {
					   m_owner->SaveScene();
				   },
				   "Ctrl+S",
				   false,
				   false,
				   false,
				   hasScene },
				 { "Save Scene As...",
				   [this]()
				   {
					   m_owner->RequestSaveSceneAs();
				   },
				   "Ctrl+Shift+S",
				   false,
				   false,
				   false,
				   hasScene },
				 { "Close Scene",
				   [this]()
				   {
					   m_owner->RequestCloseScene();
				   },
				   "Ctrl+W",
				   false,
				   false,
				   false,
				   hasScene } };
	}

	if (menu == "Project")
	{
		return { { "Project Settings...",
				   [this]()
				   {
					   m_projectSettingsPopup.Open();
				   } },
				 { "Generate C# Project",
				   [this]()
				   {
					   m_owner->GenerateCSharpProject();
				   } },
				 { "Build C# Project",
				   [this]()
				   {
					   m_owner->BuildCSharpProject();
				   } },
				 { "Reload C# Assembly",
				   [this]()
				   {
					   m_owner->ReloadCSharpAssembly();
				   } },
				 { "", {}, "", true },
				 { "Quit",
				   [this]()
				   {
					   m_owner->RequestQuit();
				   } } };
	}

	if (menu == "Editor")
	{
		return { { "Editor Settings...",
				   [this]()
				   {
					   ShowEditorSettings();
				   } },
				 { "Restore Default Layout",
				   [this]()
				   {
					   ResetNativeLayout();
				   } } };
	}

	if (menu == "View")
	{
		std::vector<MenuItem> items;
		for (EditorPanel* panel : std::initializer_list<EditorPanel*>{ m_scenePanel,
																	   m_fileSystemPanel,
																	   m_inspectorPanel,
																	   m_importPanel,
																	   m_outputPanel })
		{
			items.push_back(
				{ panel->GetTitle(),
				  [this, panel]()
				  {
					  TogglePanel(panel);
				  },
				  "",
				  false,
				  true,
				  panel->IsOpen() });
		}
		items.push_back({ "", {}, "", true });
		items.push_back(
			{ "Restore Default Layout",
			  [this]()
			  {
				  ResetNativeLayout();
			  } });
		return items;
	}

	return { { "About Ming",
			   [this]()
			   {
				   Warning("Ming", "Ming Engine - Warm Carbon");
			   } } };
}

void EditorUI::CreateRunControls()
{
	auto* actions = MemNew<HBoxContainer>();
	actions->AddThemeChangedCallback(
		[control = actions]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	m_actions = actions;
	m_nativeRoot->AddNode(actions);
	auto* workspaces = MemNew<HBoxContainer>();
	workspaces->AddThemeChangedCallback(
		[control = workspaces]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	m_workspaces = workspaces;
	m_nativeRoot->AddNode(workspaces);
	Button* twoD = EditorPopupUtils::AddButton(workspaces, "2D", {});
	twoD->SetThemeTypeVariation("EditorToolbarButton");
	twoD->SetDisabled(true);
	Button* threeD = EditorPopupUtils::AddButton(workspaces, "3D", {});
	threeD->SetThemeTypeVariation("EditorWorkspaceButton");
	threeD->SetVerticalSizeFlags(Control::Size_ShrinkCenter);
	twoD->SetVerticalSizeFlags(Control::Size_ShrinkCenter);
	auto addAction = [this, actions](char const* text, char const* icon, std::function<void()> action)
	{
		Button* button = EditorPopupUtils::AddButton(actions, text, std::move(action));
		button->SetName(icon);
		button->SetThemeTypeVariation("EditorToolbarButton");
		button->SetVerticalSizeFlags(Control::Size_ShrinkCenter);
		button->SetIcon(m_nativeRoot->GetThemeIcon(icon, "EditorIcons"));
		m_toolbarActions.push_back({ button, icon, text });
		return button;
	};
	m_playButton = addAction(
		"",
		"Play",
		[this]()
		{
			m_owner->RequestPlay();
		});
	addAction("", "Pause", {})->SetDisabled(true);
	m_stopButton = addAction(
		"",
		"Stop",
		[]()
		{
			g_app->StopGame();
		});
}

void EditorUI::CreateStatusBar()
{
	m_statusBar = MemNew<HBoxContainer>();
	m_statusBar->AddThemeChangedCallback(
		[control = m_statusBar]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("status_spacing", "Editor")));
		});
	m_nativeRoot->AddNode(m_statusBar);
	m_status = EditorPopupUtils::AddLabel(m_statusBar, "", "EditorStatusLabel");
	m_status->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_status->SetThemeTypeVariation("EditorStatusLabel");
	m_status->SetClipText(true);
	m_status->SetVerticalAlignment(VerticalAlignment::Center);
	m_statusMessage = EditorPopupUtils::AddLabel(m_statusBar, "", "EditorStatusLabel");
	m_statusMessage->SetClipText(true);
	m_statusMessage->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_statusMessage->SetVerticalAlignment(VerticalAlignment::Center);
	m_statusInfo = EditorPopupUtils::AddLabel(m_statusBar, "", "EditorStatusLabel");
	m_statusInfo->SetVerticalAlignment(VerticalAlignment::Center);
}

void EditorUI::Render(EditorUIContext& context)
{
	InitializeNative(m_owner != nullptr ? m_owner : EditorNode::Get());
	context.m_editorUI    = this;
	context.m_rootControl = m_nativeRoot;
	m_playButton->SetDisabled(g_app != nullptr && g_app->IsGameRunning());
	m_stopButton->SetDisabled(g_app == nullptr || !g_app->IsGameRunning());
	Vector2 const size(static_cast<Viewport*>(context.m_sceneTree->GetRoot())->GetOutputResolution());
	m_nativeRoot->SetColor(m_nativeRoot->GetThemeColor("background", "Editor"));
	g_engine->m_renderServer->ViewportSetClearColor(
		m_sceneView->GetSubViewport()->GetViewportRID(),
		m_nativeRoot->GetThemeColor("scene_background", "Editor"));
	float fullActionWidth = 0.f;
	for (auto const& item : m_toolbarActions)
	{
		Button* button = item.m_button;
		button->SetIcon(m_nativeRoot->GetThemeIcon(item.m_icon, "EditorIcons"));
		float const textWidth =
			g_engine->m_textServer
				->GetTextLine(button->GetFont()->GetFontRID(), item.m_text, static_cast<float>(button->GetFontSize()))
				.m_width;
		fullActionWidth += static_cast<float>(button->GetThemeConstant("icon_size")) + textWidth
						   + button->GetThemeConstant("icon_separation")
						   + button->GetThemeStyleBox("normal")->GetMinimumSize().x;
	}
	fullActionWidth += std::max(0, static_cast<int>(m_toolbarActions.size()) - 1)
					   * static_cast<HBoxContainer*>(m_actions)->GetSeparation();
	Place(m_nativeRoot, 0.f, 0.f, size.x, size.y);
	float const headerHeight = static_cast<float>(m_nativeRoot->GetThemeConstant("top_bar_height", "Editor"));
	float const statusHeight = static_cast<float>(m_nativeRoot->GetThemeConstant("status_bar_height", "Editor"));
	float const brandWidth   = static_cast<float>(m_nativeRoot->GetThemeConstant("brand_width", "Editor"));
	float const margin       = static_cast<float>(m_nativeRoot->GetThemeConstant("panel_margin", "Editor"));
	Place(
		m_beam,
		0.f,
		0.f,
		std::min(
			static_cast<float>(m_nativeRoot->GetThemeConstant("beam_max_width", "Editor")),
			std::max(
				1.f,
				size.x * 0.5f - static_cast<float>(m_nativeRoot->GetThemeConstant("beam_center_gap", "Editor")))),
		headerHeight);
	Place(m_brand, 0.f, 0.f, brandWidth, headerHeight);
	Vector2 const menuSize       = m_menuBar->GetCombinedMinimumSize();
	bool const    showActionText = brandWidth + menuSize.x + margin * 3.f + fullActionWidth <= size.x;
	for (ToolbarAction const& action : m_toolbarActions)
	{
		action.m_button->SetText(showActionText ? action.m_text : String());
	}

	Vector2 const actionSize = m_actions->GetCombinedMinimumSize();
	Place(m_menuBar, brandWidth, (headerHeight - menuSize.y) * 0.5f, menuSize.x, menuSize.y);
	Place(
		m_actions,
		size.x - actionSize.x - margin * 2.f,
		(headerHeight - actionSize.y) * 0.5f,
		actionSize.x,
		actionSize.y);
	Vector2 const workspaceSize = m_workspaces->GetCombinedMinimumSize();
	float const   workspaceLeft = (size.x - workspaceSize.x) * 0.5f;
	float const   actionLeft    = size.x - actionSize.x - margin * 2.f;
	m_workspaces->SetVisible(
		workspaceLeft > brandWidth + menuSize.x + margin && workspaceLeft + workspaceSize.x + margin < actionLeft);
	Place(m_workspaces, workspaceLeft, (headerHeight - workspaceSize.y) * 0.5f, workspaceSize.x, workspaceSize.y);
	Place(m_dockSplits[0], margin, headerHeight, size.x - margin * 2.f, size.y - headerHeight - statusHeight);
	Place(m_statusBar, margin * 2.f, size.y - statusHeight, size.x - margin * 4.f, statusHeight);
	UpdateNativeDocking();
	context.m_sceneTree->FlushContainerUpdate();
	for (EditorPanel* panel : std::initializer_list<EditorPanel*>{ m_scenePanel,
																   m_fileSystemPanel,
																   m_inspectorPanel,
																   m_importPanel,
																   m_outputPanel,
																   m_viewportPanel })
	{
		panel->Render(context);
	}

	std::string const project    = g_engine->m_fileSystem->GetResourceRoot().filename().u8string();
	std::string const scene      = m_owner->GetCurrentSceneName();
	IntVec2 const     resolution = m_sceneView->GetSubViewport()->GetOutputResolution();
	m_status->SetText(project + " / " + scene + (m_owner->IsSceneDirty() ? " *" : ""));
	m_statusInfo->SetText("D3D11    " + std::to_string(resolution.x) + " x " + std::to_string(resolution.y));
	m_projectSettingsPopup.Render(context);
	m_sceneFilePopup.Render(context);
	m_warningPopup.Render(context);
	if (m_settingsPopup != nullptr && m_settingsPopup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_settingsPopup,
			Vector2(
				static_cast<float>(m_nativeRoot->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(m_nativeRoot->GetThemeConstant("create_popup_height", "Editor"))));
	}
	if (m_unsavedPopup != nullptr && m_unsavedPopup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_unsavedPopup,
			Vector2(
				static_cast<float>(m_nativeRoot->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(m_nativeRoot->GetThemeConstant("create_popup_height", "Editor"))));
	}

	context.m_sceneTree->FlushContainerUpdate();
	SetViewportRect(m_sceneView->GetGlobalPosition(), m_sceneView->GetSize());
	context.m_isViewportImageHovered = IsSceneHovered();
	auto* root                       = static_cast<Viewport*>(context.m_sceneTree->GetRoot());
	root->UpdateGuiDrag(root->GetGuiDragPosition());
}

void EditorUI::SetNativeDragPreview(Control* source, String const& text)
{
	auto* panel = MemNew<PanelContainer>();
	panel->SetThemeSource(source);
	auto* label = EditorPopupUtils::AddLabel(panel, text);
	panel->UpdateSize(
		label->GetCombinedMinimumSize()
		+ Vector2(
			static_cast<float>(source->GetThemeConstant("spacing", "Editor")) * 2.f,
			static_cast<float>(source->GetThemeConstant("spacing", "Editor")) * 2.f));
	source->SetDragPreview(panel);
}

void EditorUI::ShowMenu(Control* source, Vector2 position, std::vector<MenuItem> const& items)
{
	FinishNativeEditing();
	if (m_menu == nullptr)
	{
		m_menu = EditorPopupUtils::Create(GetRootControl(), "", m_menuContent);
		m_menu->SetModal(false);
		static_cast<Control*>(m_menuContent->GetParent())->SetThemeTypeVariation("EditorMenuPopup");
	}

	EditorPopupUtils::ClearContent(m_menuContent);
	m_menuContent->SetSeparation(0.f);
	auto buttons = MenuBar::CreatePopupItems(
		m_menuContent,
		source,
		items,
		[this]()
		{
			m_menu->Close(false);
		});

	EditorPopupUtils::Open(
		m_menu,
		source,
		Vector2(static_cast<float>(source->GetThemeConstant("popup_minimum_width", "MenuBar")), 0.f));
	MenuBar::LayoutPopupItems(buttons);
	auto*         panel = static_cast<Control*>(m_menu->GetChildren().front());
	Vector2 const bounds(m_menu->GetViewport()->GetOutputResolution());
	panel->SetPosition(Vector2(
		std::clamp(position.x, 0.f, std::max(0.f, bounds.x - panel->GetSize().x)),
		std::clamp(position.y, 0.f, std::max(0.f, bounds.y - panel->GetSize().y))));
}

void EditorUI::SetStatusMessage(String const& message)
{
	m_statusMessage->SetText(message);
}

void EditorUI::FinishNativeEditing()
{
	if (m_inspectorPanel != nullptr)
	{
		m_inspectorPanel->FinishEditing();
	}

	if (m_nativeRoot != nullptr)
	{
		m_nativeRoot->GetViewport()->SetGuiFocus(nullptr);
	}
}

void EditorUI::AcceptUnsavedScene(EditorNode::SceneActionChoice choice)
{
	m_resolvingUnsavedPopup = true;
	m_unsavedPopup->Close();
	m_resolvingUnsavedPopup = false;
	m_owner->ResolvePendingSceneAction(choice);
}

void EditorUI::ShowUnsavedScene()
{
	FinishNativeEditing();
	m_viewportPanel->CancelNavigation();
	if (m_unsavedPopup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_unsavedPopup         = EditorPopupUtils::Create(GetRootControl(), "Unsaved Scene", content);
		m_unsavedPopup->SetClosedCallback(
			[this]()
			{
				if (!m_resolvingUnsavedPopup)
				{
					m_owner->ResolvePendingSceneAction(EditorNode::SceneActionChoice::Cancel);
				}
			});
		EditorPopupUtils::AddLabel(content, "This scene has unsaved changes. Save before continuing?");
		auto* actions = EditorPopupUtils::AddActions(content);
		EditorPopupUtils::AddButton(
			actions,
			"Don't Save",
			[this]()
			{
				AcceptUnsavedScene(EditorNode::SceneActionChoice::Discard);
			});
		EditorPopupUtils::AddButton(
			actions,
			"Cancel",
			[this]()
			{
				AcceptUnsavedScene(EditorNode::SceneActionChoice::Cancel);
			});
		Button* save = EditorPopupUtils::AddButton(
			actions,
			"Save & Continue",
			[this]()
			{
				AcceptUnsavedScene(EditorNode::SceneActionChoice::Save);
			});
		save->SetThemeTypeVariation("EditorPrimaryButton");
	}
	EditorPopupUtils::Open(
		m_unsavedPopup,
		GetRootControl(),
		Vector2(
			static_cast<float>(m_nativeRoot->GetThemeConstant("project_popup_width", "Editor")),
			static_cast<float>(m_nativeRoot->GetThemeConstant("create_popup_height", "Editor"))));
}

void EditorUI::OpenSceneFile(
	SceneFilePopup::Mode                    mode,
	VirtualPath const&                      initial,
	std::function<bool(VirtualPath const&)> accepted,
	std::function<void()>                   cancelled)
{
	FinishNativeEditing();
	m_viewportPanel->CancelNavigation();
	m_sceneFilePopup.Open(mode, initial, std::move(accepted), std::move(cancelled));
}

void EditorUI::ShowEditorSettings()
{
	if (m_settingsPopup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_settingsPopup        = EditorPopupUtils::Create(GetRootControl(), "Editor Settings", content);
		EditorPopupUtils::AddLabel(content, "Interface / Editor Scale");
		auto* scale     = MemNew<OptionButton>();
		m_settingsScale = scale;
		scale->SetName("EditorScale");
		scale->SetOptions("100%:100,125%:125,150%:150,200%:200");
		scale->SetValueNoSignal(static_cast<int>(std::round(m_owner->GetEditorScale() * 100.f)));
		scale->SetValueChangedCallback(
			[this](int value)
			{
				if (!m_owner->SetEditorScale(value / 100.f))
				{
					Warning("Editor Settings", "Could not save the editor scale.");
				}
			});
		content->AddNode(scale);
		EditorPopupUtils::AddButton(
			EditorPopupUtils::AddActions(content),
			"Close",
			[this]()
			{
				m_settingsPopup->Close();
			});
	}
	m_settingsScale->SetValueNoSignal(static_cast<int>(std::round(m_owner->GetEditorScale() * 100.f)));

	EditorPopupUtils::Open(
		m_settingsPopup,
		GetRootControl(),
		Vector2(
			static_cast<float>(m_nativeRoot->GetThemeConstant("project_popup_width", "Editor")),
			static_cast<float>(m_nativeRoot->GetThemeConstant("create_popup_height", "Editor"))));
}

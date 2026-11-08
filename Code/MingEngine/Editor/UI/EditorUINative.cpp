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
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/ProgressBar.hpp"

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
	m_playButton->SetDisabled(m_owner->IsPreparingPlay() || m_owner->IsBuildingCSharpProject()
		|| (g_app != nullptr && g_app->IsGameRunning()));
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
	m_createScriptPopup.Render(context);
	m_warningPopup.Render(context);
	if (IsBuildProgressVisible())
	{
		EditorPopupUtils::Layout(m_buildPopup,
			Vector2(static_cast<float>(m_nativeRoot->GetThemeConstant("build_popup_width", "Editor")), 0.f));
	}
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
				static_cast<float>(m_nativeRoot->GetThemeConstant("unsaved_popup_width", "Editor")),
				0.f));
	}

	context.m_sceneTree->FlushContainerUpdate();
	SetViewportRect(m_sceneView->GetGlobalPosition(), m_sceneView->GetSize());
	context.m_isViewportImageHovered = IsSceneHovered();
	auto* root                       = static_cast<Viewport*>(context.m_sceneTree->GetRoot());
	root->UpdateGuiDrag(root->GetGuiDragPosition());
}

void EditorUI::SetNativeDragPreview(Control* source, String const& text)
{
	auto* panel = MemNew<ColorRect>();
	panel->SetThemeSource(source);
	auto* row = MemNew<HBoxContainer>();
	panel->AddNode(row);
	VirtualPath path;
	bool const script = VirtualPath::TryParse(text.ToUtf8(), path) && path.HasExtension(".cs");
	if (script)
	{
		auto* icon = MemNew<TextureRect>();
		icon->SetTexture(source->GetThemeIcon("CSharpScript", "EditorIcons"));
		icon->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
		float const iconSize = static_cast<float>(source->GetThemeConstant("icon_size", "Button"));
		icon->SetCustomMinimumSize(Vector2(iconSize, iconSize));
		row->AddNode(icon);
	}

	EditorPopupUtils::AddLabel(row, script ? String(path.GetFileName()) : text);
	float const padding = static_cast<float>(source->GetThemeConstant("spacing", "Editor"));
	Vector2 const contentSize = row->GetCombinedMinimumSize();
	row->SetPosition(Vector2(padding, padding));
	row->UpdateSize(contentSize);
	panel->UpdateSize(contentSize + Vector2(padding * 2.f, padding * 2.f));
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

	for (size_t i = 1; i < m_menuLevels.size(); ++i)
	{
		m_menuLevels[i].m_panel->SetVisible(false);
		m_menuLevels[i].m_panel->QueueFree();
	}

	m_menuLevels.clear();
	EditorPopupUtils::ClearContent(m_menuContent);
	m_menu->SetKeyCallback([this](InputEvent const& event) { HandleMenuKey(event); });
	m_menuContent->SetSeparation(0.f);
	auto buttons = MenuBar::CreatePopupItems(
		m_menuContent,
		source,
		items,
		[this]()
		{
			m_menu->Close(false);
		},
		[this](Button* parent, std::vector<MenuItem> const& children) { OpenSubmenu(parent, children); });
	m_menuLevels.push_back({ static_cast<Control*>(m_menuContent->GetParent()), nullptr, buttons, items });

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

void EditorUI::ShowBuildProgress(String const& message)
{
	if (m_buildPopup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_buildPopup = EditorPopupUtils::Create(GetRootControl(), "C# Build", content, false);
		m_buildPopup->SetName("BuildProgressPopup");
		m_buildMessage = EditorPopupUtils::AddLabel(content, message);
		m_buildProgress = MemNew<ProgressBar>();
		m_buildProgress->SetName("BuildProgress");
		m_buildProgress->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		content->AddNode(m_buildProgress);
	}

	m_buildMessage->SetText(message);
	m_buildProgress->SetIndeterminate(true);
	if (!m_buildPopup->IsVisible())
	{
		EditorPopupUtils::Open(m_buildPopup, GetRootControl(),
			Vector2(static_cast<float>(m_nativeRoot->GetThemeConstant("build_popup_width", "Editor")), 0.f));
	}
}

void EditorUI::HideBuildProgress()
{
	if (m_buildPopup != nullptr)
	{
		m_buildProgress->SetIndeterminate(false);
		m_buildPopup->Close();
	}
}

bool EditorUI::IsBuildProgressVisible() const
{
	return m_buildPopup != nullptr && m_buildPopup->IsVisible();
}

void EditorUI::AcceptUnsavedScene(EditorNode::SceneActionChoice choice)
{
	m_resolvingUnsavedPopup = true;
	m_unsavedPopup->Close();
	m_resolvingUnsavedPopup = false;
	m_owner->ResolvePendingSceneAction(choice);
}

void EditorUI::ShowUnsavedScene(String const& message, String const& question, String const& action)
{
	FinishNativeEditing();
	m_viewportPanel->CancelNavigation();
	if (m_unsavedPopup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_unsavedPopup         = EditorPopupUtils::Create(GetRootControl(), "Please Confirm...", content);
		m_unsavedPopup->SetClosedCallback(
			[this]()
			{
				if (!m_resolvingUnsavedPopup)
				{
					m_owner->ResolvePendingSceneAction(EditorNode::SceneActionChoice::Cancel);
				}
			});
		m_unsavedMessage = EditorPopupUtils::AddLabel(content, "");
		m_unsavedMessage->SetAutowrapMode(AutowrapMode::WordSmart);
		m_unsavedQuestion = EditorPopupUtils::AddLabel(content, "");
		auto* actions = MemNew<HBoxContainer>();
		m_unsavedActions = actions;
		actions->AddThemeChangedCallback([actions]()
		{
			actions->SetSeparation(static_cast<float>(actions->GetThemeConstant("content_padding", "Editor")) * 2.f);
		});
		actions->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		content->AddNode(actions);
		m_unsavedSave = EditorPopupUtils::AddButton(actions, "", [this]()
		{
			AcceptUnsavedScene(EditorNode::SceneActionChoice::Save);
		});
		m_unsavedSave->SetThemeTypeVariation("EditorPrimaryButton");
		Button* cancel = EditorPopupUtils::AddButton(actions, "Cancel", [this]()
		{
			AcceptUnsavedScene(EditorNode::SceneActionChoice::Cancel);
		});
		Button* discard = EditorPopupUtils::AddButton(actions, "Don't Save", [this]()
		{
			AcceptUnsavedScene(EditorNode::SceneActionChoice::Discard);
		});
		for (Button* button : { m_unsavedSave, cancel, discard })
		{
			button->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		}

	}
	m_unsavedMessage->SetText(message);
	m_unsavedQuestion->SetText(question);
	m_unsavedSave->SetText(action);
	float buttonWidth = 0.f;
	for (Node* child : m_unsavedActions->GetChildren())
	{
		auto* button = static_cast<Button*>(child);
		button->SetCustomMinimumSize(Vector2::Zero);
		buttonWidth = std::max(buttonWidth, button->GetCombinedMinimumSize().x);
	}

	for (Node* child : m_unsavedActions->GetChildren())
	{
		static_cast<Button*>(child)->SetCustomMinimumSize(Vector2(buttonWidth, 0.f));
	}

	EditorPopupUtils::Open(
		m_unsavedPopup,
		GetRootControl(),
		Vector2(
			static_cast<float>(m_nativeRoot->GetThemeConstant("unsaved_popup_width", "Editor")),
			0.f));
	m_unsavedSave->GrabFocus();
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

void EditorUI::CreateScript(VirtualPath const& directory, std::string const& base, std::function<void(VirtualPath const&)> created)
{
	FinishNativeEditing();
	m_createScriptPopup.Open(directory, base, std::move(created));
}

void EditorUI::OpenSubmenu(Button* parent, std::vector<MenuItem> const& items)
{
	size_t level = 0;
	for (; level < m_menuLevels.size(); ++level)
	{
		auto const& buttons = m_menuLevels[level].m_buttons;
		if (std::find(buttons.begin(), buttons.end(), parent) != buttons.end())
		{
			break;
		}
	}

	if (level == m_menuLevels.size())
	{
		return;
	}

	if (level + 1 < m_menuLevels.size() && m_menuLevels[level + 1].m_parent == parent)
	{
		return;
	}

	while (m_menuLevels.size() > level + 1)
	{
		m_menuLevels.back().m_panel->SetVisible(false);
		m_menuLevels.back().m_panel->QueueFree();
		m_menuLevels.pop_back();
	}

	auto* panel = MemNew<PanelContainer>();
	panel->SetThemeTypeVariation("EditorMenuPopup");
	m_menu->AddNode(panel);
	auto* content = MemNew<VBoxContainer>();
	content->SetSeparation(0.f);
	panel->AddNode(content);
	auto buttons = MenuBar::CreatePopupItems(content, parent, items, [this]() { m_menu->Close(false); },
		[this](Button* button, std::vector<MenuItem> const& children) { OpenSubmenu(button, children); });
	Vector2 size = panel->GetCombinedMinimumSize();
	size.x = std::max(size.x, static_cast<float>(parent->GetThemeConstant("popup_minimum_width", "MenuBar")));
	Vector2 const bounds(m_menu->GetViewport()->GetOutputResolution());
	Vector2 origin = parent->GetGlobalPosition() + Vector2(parent->GetSize().x, 0.f);
	if (origin.x + size.x > bounds.x)
	{
		origin.x = m_menuLevels[level].m_panel->GetGlobalPosition().x - size.x;
	}

	panel->SetPosition(Vector2(std::clamp(origin.x, 0.f, std::max(0.f, bounds.x - size.x)),
		std::clamp(origin.y, 0.f, std::max(0.f, bounds.y - size.y))));
	panel->UpdateSize(size);
	m_owner->GetSceneTree()->FlushContainerUpdate();
	MenuBar::LayoutPopupItems(buttons);
	m_menuLevels.push_back({ panel, parent, buttons, items });
}

void EditorUI::HandleMenuPointer(InputEvent const& event)
{
	if (m_menu == nullptr || !m_menu->IsVisible() || event.m_type != InputEventType::MouseMotion)
	{
		return;
	}

	for (size_t depth = m_menuLevels.size(); depth > 0; --depth)
	{
		auto const buttons = m_menuLevels[depth - 1].m_buttons;
		auto const items = m_menuLevels[depth - 1].m_items;
		size_t index = 0;
		for (auto const& item : items)
		{
			if (item.m_separator)
			{
				continue;
			}

			Button* button = buttons[index++];
			if (!button->IsPointInside(event.m_position - button->GetGlobalPosition()))
			{
				continue;
			}

			if (!button->IsDisabled() && !item.m_children.empty())
			{
				OpenSubmenu(button, item.m_children);
			}
			else
			{
				while (m_menuLevels.size() > depth)
				{
					m_menuLevels.back().m_panel->SetVisible(false);
					m_menuLevels.back().m_panel->QueueFree();
					m_menuLevels.pop_back();
				}
			}

			return;
		}
	}
}

void EditorUI::HandleMenuKey(InputEvent const& event)
{
	if (event.m_type != InputEventType::Key || m_menuLevels.empty())
	{
		return;
	}

	Control* focus = m_menu->GetViewport()->GetGuiFocus();
	size_t depth = m_menuLevels.size() - 1;
	for (size_t i = 0; i < m_menuLevels.size(); ++i)
	{
		auto const& buttons = m_menuLevels[i].m_buttons;
		if (std::find(buttons.begin(), buttons.end(), focus) != buttons.end())
		{
			depth = i;
			break;
		}
	}

	if (!event.m_pressed)
	{
		auto const& buttons = m_menuLevels[depth].m_buttons;
		if (focus != nullptr && std::find(buttons.begin(), buttons.end(), focus) != buttons.end())
		{
			focus->GuiInput(event);
		}

		return;
	}

	if (event.m_keyCode == GLFW_KEY_ESCAPE || event.m_keyCode == GLFW_KEY_LEFT)
	{
		if (m_menuLevels.size() > 1)
		{
			Button* parent = m_menuLevels.back().m_parent;
			m_menuLevels.back().m_panel->SetVisible(false);
			m_menuLevels.back().m_panel->QueueFree();
			m_menuLevels.pop_back();
			parent->GrabFocus();
		}
		else
		{
			m_menu->Close();
		}

		return;
	}

	auto buttons = m_menuLevels[depth].m_buttons;
	if (event.m_keyCode == GLFW_KEY_UP || event.m_keyCode == GLFW_KEY_DOWN)
	{
		int index = -1;
		for (int i = 0; i < static_cast<int>(buttons.size()); ++i)
		{
			if (buttons[i] == focus)
			{
				index = i;
			}
		}

		for (size_t i = 0; i < buttons.size(); ++i)
		{
			index = (index + (event.m_keyCode == GLFW_KEY_DOWN ? 1 : static_cast<int>(buttons.size()) - 1)
				+ static_cast<int>(buttons.size())) % static_cast<int>(buttons.size());
			if (!buttons[index]->IsDisabled())
			{
				buttons[index]->GrabFocus();
				break;
			}
		}

		return;
	}

	if (event.m_keyCode == GLFW_KEY_RIGHT || event.m_keyCode == GLFW_KEY_ENTER || event.m_keyCode == GLFW_KEY_KP_ENTER)
	{
		size_t index = 0;
		for (auto const& item : m_menuLevels[depth].m_items)
		{
			if (item.m_separator)
			{
				continue;
			}

			Button* button = buttons[index++];
			if (button == focus && !item.m_children.empty())
			{
				auto children = item.m_children;
				OpenSubmenu(button, children);
				for (Button* child : m_menuLevels.back().m_buttons)
				{
					if (!child->IsDisabled())
					{
						child->GrabFocus();
						break;
					}
				}

				return;
			}
		}
	}

	if (focus != nullptr && std::find(buttons.begin(), buttons.end(), focus) != buttons.end())
	{
		focus->GuiInput(event);
	}
}

void EditorUI::OpenScriptFile(std::function<void(VirtualPath const&)> selected)
{
	FinishNativeEditing();
	if (m_scriptFilePopup == nullptr)
	{
		m_scriptFilePopup = EditorPopupUtils::Create(GetRootControl(), "Load C# Script", m_scriptFileContent);
	}

	EditorPopupUtils::ClearContent(m_scriptFileContent);
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetCustomMinimumSize(Vector2(0.f, static_cast<float>(GetRootControl()->GetThemeConstant("create_popup_height", "Editor"))));
	m_scriptFileContent->AddNode(scroll);
	auto* list = MemNew<VBoxContainer>();
	scroll->AddNode(list);
	std::error_code error;
	bool found = false;
	for (std::filesystem::recursive_directory_iterator it(g_engine->m_fileSystem->GetResourceRoot(), error), end;
		!error && it != end; it.increment(error))
	{
		if (it->is_directory(error) && (it->path().filename() == ".ming" || it->path().filename() == "obj" || it->path().filename() == "bin"))
		{
			it.disable_recursion_pending();
		}
		else if (it->path().extension() == ".cs")
		{
			VirtualPath path;
			if (g_engine->m_fileSystem->TryToVirtualPath(it->path(), path))
			{
				found = true;
				EditorPopupUtils::AddButton(list, path.GetString(), [this, selected, path]()
				{
					m_scriptFilePopup->Close();
					selected(path);
				});
			}
		}
	}

	if (!found)
	{
		EditorPopupUtils::AddLabel(list, "No C# scripts in this project.");
	}

	EditorPopupUtils::AddButton(EditorPopupUtils::AddActions(m_scriptFileContent), "Cancel", [this]() { m_scriptFilePopup->Close(); });
	EditorPopupUtils::Open(m_scriptFilePopup, GetRootControl(),
		Vector2(static_cast<float>(GetRootControl()->GetThemeConstant("project_popup_width", "Editor")), 0.f));
}

#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"

#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"

#include <algorithm>
#include <cctype>

void ProjectSettingsPopup::Open()
{
	Ref<ProjectSettings> const settings = ProjectSettings::Get();
	m_startScenePath   = settings->m_startScenePath.GetString();
	m_autoLoads        = settings->m_autoLoads;
	m_refreshAutoloads = true;
	m_openRequested    = true;
	SetError("");
	if (m_autoloadPath != nullptr)
	{
		m_autoloadPath->SetText("");
		m_autoloadName->SetText("");
	}
}

bool ProjectSettingsPopup::IsOpen() const
{
	return m_openRequested || (m_popup != nullptr && m_popup->IsVisible());
}

void ProjectSettingsPopup::Render(EditorUIContext& context)
{
	if (m_openRequested)
	{
		PrepareFields(context);
		m_openRequested = false;
		m_mainScene->SetText(m_startScenePath);
		EditorPopupUtils::Open(
			m_popup, context.m_rootControl,
			Vector2(static_cast<float>(m_content->GetThemeConstant("project_settings_width", "Editor")),
				static_cast<float>(m_content->GetThemeConstant("project_settings_height", "Editor"))));
		if (m_filter->IsVisibleInHierarchy())
		{
			m_filter->GrabFocus();
		}
	}

	if (m_popup != nullptr && m_popup->IsVisible())
	{
		if (m_refreshAutoloads)
		{
			RefreshAutoloads();
		}

		EditorPopupUtils::Layout(
			m_popup,
			Vector2(static_cast<float>(m_content->GetThemeConstant("project_settings_width", "Editor")),
				static_cast<float>(m_content->GetThemeConstant("project_settings_height", "Editor"))));
	}
}

bool ProjectSettingsPopup::Confirm()
{
	if (m_mainScene != nullptr)
	{
		m_startScenePath = m_mainScene->GetText().ToUtf8();
	}

	std::string error;
	Ref<ProjectSettings> draft = CreateRef<ProjectSettings>();
	if (!ProjectSettings::ParseMainScenePath(m_startScenePath, draft->m_startScenePath, error)
		|| !ProjectSettings::ValidateAutoloads(m_autoLoads, error))
	{
		SetError(error);
		return false;
	}

	if (draft->m_startScenePath.IsValid()
		&& !Ref<PackedScene>(ResourceLoader::LoadUncached(draft->m_startScenePath)).IsValid())
	{
		SetError("Main Scene must refer to an existing, valid scene.");
		return false;
	}

	draft->m_autoLoads = m_autoLoads;
	if (!ResourceSaver::Save(ProjectSettings::GetSettingsPath(), draft))
	{
		SetError("Could not save project settings to res://project.ming.");
		return false;
	}

	Ref<ProjectSettings> settings = ProjectSettings::Get();
	settings->m_startScenePath = draft->m_startScenePath;
	settings->m_autoLoads = draft->m_autoLoads;
	SetError("");
	if (m_popup != nullptr)
	{
		m_popup->Close();
	}

	return true;
}

void ProjectSettingsPopup::Cancel()
{
	m_openRequested = false;
	if (m_popup != nullptr)
	{
		m_popup->Close();
	}
}

void ProjectSettingsPopup::SetError(std::string const& error)
{
	m_error = error;
	if (m_errorLabel != nullptr)
	{
		m_errorLabel->SetText(error);
		m_errorLabel->SetVisible(!error.empty());
	}
}

void ProjectSettingsPopup::Browse(EditorUIContext const& context, bool autoload)
{
	m_startScenePath = m_mainScene->GetText().ToUtf8();
	VirtualPath initial;
	VirtualPath::TryParse(autoload ? m_autoloadPath->GetText().ToUtf8() : m_startScenePath, initial);
	m_popup->Close();
	context.m_editorUI->OpenSceneFile(
		autoload ? SceneFilePopup::Mode::Autoload : SceneFilePopup::Mode::MainScene,
		initial,
		[this, autoload](VirtualPath const& path)
		{
			if (autoload)
			{
				m_autoloadPath->SetText(path.GetString());
				if (m_autoloadName->GetText().IsEmpty())
				{
					m_autoloadName->SetText(std::filesystem::u8path(path.GetFileName()).stem().u8string());
				}
			}
			else
			{
				m_startScenePath = path.GetString();
			}

			m_openRequested = true;
			return true;
		},
		[this]()
		{
			m_openRequested = true;
		});
}

void ProjectSettingsPopup::AddAutoload(FileSystem const& fileSystem)
{
	VirtualPath path;
	if (!VirtualPath::TryParse(m_autoloadPath->GetText().ToUtf8(), path))
	{
		SetError("Select an autoload scene (.tscn) or C# script (.cs) inside res://.");
		return;
	}

	std::vector<ProjectSettings::AutoLoad> entries = m_autoLoads;
	entries.push_back({ m_autoloadName->GetText().ToUtf8(), path });
	std::string error;
	if (!ProjectSettings::ValidateAutoloads(entries, error))
	{
		SetError(error);
		return;
	}

	std::filesystem::path physical;
	std::error_code fileError;
	if (!fileSystem.TryGetPhysicalPath(path, physical) || !std::filesystem::is_regular_file(physical, fileError))
	{
		SetError("Autoload file does not exist: " + path.GetString());
		return;
	}

	if (path.HasExtension(".tscn") && !Ref<PackedScene>(ResourceLoader::LoadUncached(path)).IsValid())
	{
		SetError("Autoload must refer to a valid scene: " + path.GetString());
		return;
	}

	m_autoLoads = std::move(entries);
	m_refreshAutoloads = true;
	m_autoloadPath->SetText("");
	m_autoloadName->SetText("");
	SetError("");
}

void ProjectSettingsPopup::RefreshAutoloads()
{
	m_refreshAutoloads = false;
	EditorPopupUtils::ClearContent(m_autoloadRows);
	m_autoloadRows->SetVisible(!m_autoLoads.empty());
	m_autoloadEmpty->SetVisible(m_autoLoads.empty());
	if (m_autoLoads.empty())
	{
		return;
	}

	for (size_t index = 0; index < m_autoLoads.size(); ++index)
	{
		auto* row = MemNew<HBoxContainer>();
		row->SetThemeTypeVariation("EditorSettingsGroup");
		m_autoloadRows->AddNode(row);

		auto* name = MemNew<LineEdit>();
		name->SetName("AutoloadName_" + std::to_string(index));
		name->SetThemeTypeVariation("EditorSettingsEdit");
		name->SetMinimumCharacterWidth(14);
		name->SetText(m_autoLoads[index].m_name);
		row->AddNode(name);
		name->AddThemeChangedCallback(
			[row, name, titleID = m_autoloadNameTitle->GetObjectID()]()
			{
				if (auto* title = ObjectDatabase::GetInstance<Label>(titleID); title != nullptr && !row->IsQueuedForDeletion())
				{
					title->SetCustomMinimumSize(Vector2(name->GetCombinedMinimumSize().x, 0.f));
				}
			});
		name->SetTextChangedCallback(
			[this, index](String const& text)
			{
				if (!m_refreshAutoloads)
				{
					m_autoLoads[index].m_name = text.ToUtf8();
				}
			});

		auto* path = MemNew<LineEdit>();
		path->SetName("AutoloadPath_" + std::to_string(index));
		path->SetThemeTypeVariation("EditorSettingsEdit");
		path->SetMinimumCharacterWidth(1);
		path->SetEditable(false);
		path->SetText(m_autoLoads[index].m_path.GetString());
		path->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		row->AddNode(path);

		auto* actions = MemNew<HBoxContainer>();
		actions->SetThemeTypeVariation("EditorSettingsRows");
		row->AddNode(actions);
		for (bool up : { true, false })
		{
			Button* move = EditorPopupUtils::AddButton(
				actions, "",
				[this, index, up]()
				{
					if (!m_refreshAutoloads)
					{
						std::swap(m_autoLoads[index], m_autoLoads[up ? index - 1 : index + 1]);
						m_refreshAutoloads = true;
					}
				});
			move->SetName(up ? "MoveAutoloadUp" : "MoveAutoloadDown");
			move->SetThemeTypeVariation("EditorPanelAction");
			move->SetIcon(m_content->GetThemeIcon(up ? "ArrowUp" : "ArrowDown", "EditorIcons"));
			move->SetDisabled(up ? index == 0 : index + 1 == m_autoLoads.size());
		}

		Button* remove = EditorPopupUtils::AddButton(
			actions, "",
			[this, index]()
			{
				if (!m_refreshAutoloads)
				{
					m_autoLoads.erase(m_autoLoads.begin() + index);
					m_refreshAutoloads = true;
					SetError("");
				}
			});
		remove->SetName("RemoveAutoload");
		remove->SetThemeTypeVariation("EditorPanelAction");
		remove->SetIcon(m_content->GetThemeIcon("Remove", "EditorIcons"));
		remove->AddThemeChangedCallback(
			[row, actions, titleID = m_autoloadActionsTitle->GetObjectID()]()
			{
				if (auto* title = ObjectDatabase::GetInstance<Label>(titleID); title != nullptr && !row->IsQueuedForDeletion())
				{
					title->SetCustomMinimumSize(Vector2(actions->GetCombinedMinimumSize().x, 0.f));
				}
			});
	}
}

void ProjectSettingsPopup::PrepareFields(EditorUIContext& context)
{
	if (m_popup != nullptr)
	{
		return;
	}

	m_popup = EditorPopupUtils::Create(context.m_rootControl, "Project Settings", m_content);
	auto* tabs = MemNew<TabContainer>();
	tabs->SetName("ProjectSettingsTabs");
	tabs->SetThemeTypeVariation("EditorSettingsTabs");
	tabs->SetVerticalSizeFlags(Control::Size_ExpandFill);
	m_content->AddNode(tabs);

	auto* generalMargin = MemNew<MarginContainer>();
	generalMargin->SetThemeTypeVariation("EditorSettingsPage");
	tabs->AddPage("General", generalMargin);
	auto* general = MemNew<VBoxContainer>();
	general->SetThemeTypeVariation("EditorSettingsGroup");
	generalMargin->AddNode(general);

	auto* search = MemNew<HBoxContainer>();
	general->AddNode(search);
	m_filter = MemNew<LineEdit>();
	m_filter->SetName("FilterSettings");
	m_filter->SetPlaceholder("Filter Settings");
	m_filter->SetMinimumCharacterWidth(1);
	m_filter->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_filter->SetIcon(m_content->GetThemeIcon("Search", "EditorIcons"));
	search->AddNode(m_filter);

	auto* body = MemNew<HBoxContainer>();
	body->SetThemeTypeVariation("EditorSettingsSection");
	body->SetVerticalSizeFlags(Control::Size_ExpandFill);
	general->AddNode(body);
	auto* sidebar = MemNew<PanelContainer>();
	sidebar->SetThemeTypeVariation("EditorDialogSidebar");
	sidebar->AddThemeChangedCallback(
		[sidebar]()
		{
			sidebar->SetCustomMinimumSize(
				Vector2(static_cast<float>(sidebar->GetThemeConstant("node_sidebar_width", "Editor")), 0.f));
		});
	body->AddNode(sidebar);
	auto* navigation = MemNew<Tree>();
	navigation->SetName("SettingsCategories");
	navigation->SetItems({ { 1, 0, "Application" }, { 2, 1, "Run" } });
	navigation->Select(2);
	sidebar->AddNode(navigation);

	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetFollowFocus(true);
	scroll->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	body->AddNode(scroll);
	auto* fieldsMargin = MemNew<MarginContainer>();
	fieldsMargin->SetThemeTypeVariation("EditorSettingsPage");
	scroll->AddNode(fieldsMargin);
	auto* fields = MemNew<VBoxContainer>();
	fields->SetName("RunSettings");
	fields->SetThemeTypeVariation("EditorSettingsSection");
	fieldsMargin->AddNode(fields);

	auto* heading = MemNew<VBoxContainer>();
	heading->SetThemeTypeVariation("EditorSettingsRows");
	fields->AddNode(heading);
	EditorPopupUtils::AddLabel(heading, "Run", "EditorSettingsTitle");
	Label* description = EditorPopupUtils::AddLabel(heading,
		"Choose the scene launched when the project starts.", "EditorStatusLabel");
	description->SetAutowrapMode(AutowrapMode::WordSmart);

	auto* property = MemNew<HBoxContainer>();
	property->SetThemeTypeVariation("EditorSettingsGroup");
	fields->AddNode(property);
	Label* label = EditorPopupUtils::AddLabel(property, "Main Scene");
	label->SetVerticalAlignment(VerticalAlignment::Center);
	m_mainScene = MemNew<LineEdit>();
	m_mainScene->SetName("ProjectMainScene");
	m_mainScene->SetThemeTypeVariation("EditorSettingsEdit");
	m_mainScene->SetMinimumCharacterWidth(1);
	m_mainScene->SetPlaceholder("res://Scenes/Main.tscn");
	m_mainScene->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	property->AddNode(m_mainScene);
	Button* browse = EditorPopupUtils::AddButton(property, "",
		[this, context]()
		{
			Browse(context, false);
		});
	browse->SetName("BrowseMainScene");
	browse->SetIcon(m_content->GetThemeIcon("Folder", "EditorIcons"));

	auto* fieldSpacer = MemNew<Control>();
	fieldSpacer->SetMouseFilter(Control::MouseFilter::Ignore);
	fieldSpacer->SetVerticalSizeFlags(Control::Size_ExpandFill);
	fields->AddNode(fieldSpacer);

	m_filter->SetTextChangedCallback(
		[fields, navigation](String const& text)
		{
			std::string filter = text.ToUtf8();
			std::transform(filter.begin(), filter.end(), filter.begin(),
				[](unsigned char ch)
				{
					return static_cast<char>(std::tolower(ch));
				});
			bool const matches = filter.empty()
				|| std::string("application run main scene start scene").find(filter) != std::string::npos;
			fields->SetVisible(matches);
			navigation->SetItems({ { 1, 0, "Application", matches }, { 2, 1, "Run", matches } }, !filter.empty());
			navigation->Select(2);
		});

	auto* globalsMargin = MemNew<MarginContainer>();
	globalsMargin->SetThemeTypeVariation("EditorSettingsPage");
	tabs->AddPage("Globals", globalsMargin);
	auto* autoload = MemNew<VBoxContainer>();
	autoload->SetThemeTypeVariation("EditorSettingsSection");
	globalsMargin->AddNode(autoload);

	auto* autoloadHeading = MemNew<VBoxContainer>();
	autoloadHeading->SetThemeTypeVariation("EditorSettingsRows");
	autoload->AddNode(autoloadHeading);
	EditorPopupUtils::AddLabel(autoloadHeading, "Autoload", "EditorSettingsTitle");
	Label* hint = EditorPopupUtils::AddLabel(autoloadHeading,
		"Loaded in order before the main scene. Changes apply the next time the game starts.", "EditorStatusLabel");
	hint->SetAutowrapMode(AutowrapMode::WordSmart);

	auto* add = MemNew<HBoxContainer>();
	add->SetThemeTypeVariation("EditorSettingsGroup");
	autoload->AddNode(add);
	auto* pathColumn = MemNew<VBoxContainer>();
	pathColumn->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	add->AddNode(pathColumn);
	EditorPopupUtils::AddLabel(pathColumn, "Path", "EditorPropertyTitle");
	auto* pathInput = MemNew<HBoxContainer>();
	pathInput->SetThemeTypeVariation("EditorSettingsGroup");
	pathColumn->AddNode(pathInput);
	m_autoloadPath = MemNew<LineEdit>();
	m_autoloadPath->SetName("NewAutoloadPath");
	m_autoloadPath->SetThemeTypeVariation("EditorSettingsEdit");
	m_autoloadPath->SetMinimumCharacterWidth(1);
	m_autoloadPath->SetPlaceholder("res://Scripts/Session.cs");
	m_autoloadPath->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	pathInput->AddNode(m_autoloadPath);
	Button* select = EditorPopupUtils::AddButton(pathInput, "",
		[this, context]()
		{
			Browse(context, true);
		});
	select->SetName("BrowseAutoload");
	select->SetIcon(m_content->GetThemeIcon("Folder", "EditorIcons"));

	auto* nameColumn = MemNew<VBoxContainer>();
	add->AddNode(nameColumn);
	EditorPopupUtils::AddLabel(nameColumn, "Node Name", "EditorPropertyTitle");
	m_autoloadName = MemNew<LineEdit>();
	m_autoloadName->SetName("NewAutoloadName");
	m_autoloadName->SetThemeTypeVariation("EditorSettingsEdit");
	m_autoloadName->SetMinimumCharacterWidth(14);
	nameColumn->AddNode(m_autoloadName);

	auto* addColumn = MemNew<VBoxContainer>();
	add->AddNode(addColumn);
	auto* addSpacer = MemNew<Control>();
	addSpacer->SetMouseFilter(Control::MouseFilter::Ignore);
	addSpacer->SetVerticalSizeFlags(Control::Size_ExpandFill);
	addColumn->AddNode(addSpacer);
	Button* addButton = EditorPopupUtils::AddButton(addColumn, "Add",
		[this, fileSystem = context.m_fileSystem]()
		{
			AddAutoload(*fileSystem);
		});
	addButton->SetName("AddAutoload");

	auto* table = MemNew<PanelContainer>();
	table->SetThemeTypeVariation("EditorSettingsTable");
	table->SetVerticalSizeFlags(Control::Size_ExpandFill);
	autoload->AddNode(table);
	auto* list = MemNew<ScrollContainer>();
	list->SetFollowFocus(true);
	list->SetName("AutoloadList");
	table->AddNode(list);
	auto* tableBody = MemNew<VBoxContainer>();
	tableBody->SetThemeTypeVariation("EditorSettingsGroup");
	list->AddNode(tableBody);
	auto* header = MemNew<HBoxContainer>();
	header->SetThemeTypeVariation("EditorSettingsGroup");
	tableBody->AddNode(header);
	m_autoloadNameTitle = EditorPopupUtils::AddLabel(header, "Name", "EditorStatusLabel");
	m_autoloadNameTitle->SetName("AutoloadNameHeader");
	m_autoloadName->AddThemeChangedCallback(
		[name = m_autoloadName, titleID = m_autoloadNameTitle->GetObjectID()]()
		{
			if (auto* title = ObjectDatabase::GetInstance<Label>(titleID))
			{
				title->SetCustomMinimumSize(Vector2(name->GetCombinedMinimumSize().x, 0.f));
			}
		});
	Label* pathTitle = EditorPopupUtils::AddLabel(header, "Path", "EditorStatusLabel");
	pathTitle->SetName("AutoloadPathHeader");
	pathTitle->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_autoloadActionsTitle = EditorPopupUtils::AddLabel(header, "Actions", "EditorStatusLabel");
	m_autoloadActionsTitle->SetName("AutoloadActionsHeader");
	m_autoloadActionsTitle->SetHorizontalAlignment(HorizontalAlignment::Center);

	m_autoloadRows = MemNew<VBoxContainer>();
	m_autoloadRows->SetThemeTypeVariation("EditorSettingsRows");
	m_autoloadRows->SetName("AutoloadEntries");
	tableBody->AddNode(m_autoloadRows);

	m_autoloadEmpty = MemNew<VBoxContainer>();
	m_autoloadEmpty->SetName("AutoloadEmptyState");
	m_autoloadEmpty->SetThemeTypeVariation("EditorSettingsGroup");
	m_autoloadEmpty->SetVerticalSizeFlags(Control::Size_ExpandFill);
	tableBody->AddNode(m_autoloadEmpty);
	for (int index = 0; index < 2; ++index)
	{
		auto* spacer = MemNew<Control>();
		spacer->SetMouseFilter(Control::MouseFilter::Ignore);
		spacer->SetVerticalSizeFlags(Control::Size_ExpandFill);
		m_autoloadEmpty->AddNode(spacer);
		if (index == 0)
		{
			Label* emptyTitle = EditorPopupUtils::AddLabel(m_autoloadEmpty, "No autoloads configured.");
			emptyTitle->SetHorizontalAlignment(HorizontalAlignment::Center);
			Label* emptyHint = EditorPopupUtils::AddLabel(m_autoloadEmpty,
				"Add a scene or C# script above.", "EditorStatusLabel");
			emptyHint->SetHorizontalAlignment(HorizontalAlignment::Center);
			emptyHint->SetAutowrapMode(AutowrapMode::WordSmart);
		}
	}

	m_errorLabel = EditorPopupUtils::AddLabel(m_content, "", "EditorErrorLabel");
	m_errorLabel->SetAutowrapMode(AutowrapMode::WordSmart);
	m_errorLabel->SetVisible(false);

	auto* buttons = EditorPopupUtils::AddActions(m_content);
	Button* save = EditorPopupUtils::AddButton(buttons, "Save",
		[this]()
		{
			Confirm();
		});
	save->SetName("SaveProjectSettings");
	save->SetThemeTypeVariation("EditorPrimaryButton");

	Button* cancel = EditorPopupUtils::AddButton(buttons, "Cancel",
		[this]()
		{
			Cancel();
		});
	cancel->SetName("CancelProjectSettings");
}

bool ProjectSettingsPopup::HasChanges() const
{
	if (!IsOpen())
	{
		return false;
	}

	Ref<ProjectSettings> settings = ProjectSettings::Get();
	std::string const path = m_mainScene != nullptr ? m_mainScene->GetText().ToUtf8() : m_startScenePath;
	if (path != settings->m_startScenePath.GetString() || m_autoLoads.size() != settings->m_autoLoads.size())
	{
		return true;
	}

	for (size_t i = 0; i < m_autoLoads.size(); ++i)
	{
		if (m_autoLoads[i].m_name != settings->m_autoLoads[i].m_name || m_autoLoads[i].m_path != settings->m_autoLoads[i].m_path)
		{
			return true;
		}
	}

	return false;
}

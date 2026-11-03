#include "MingEngine/Editor/UI/Popup/CreateNodePopup.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/nlohmann/json.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <tuple>

namespace
{
bool CanCreateClass(ClassInfo const* info)
{
	return info != nullptr && info->m_apiType == ApiType::Runtime && !info->m_isVirtual && info->m_creator
		   && ClassDatabase::IsSubclassOf(info->m_className, "Node");
}

std::string Lower(std::string text)
{
	std::transform(
		text.begin(),
		text.end(),
		text.begin(),
		[](unsigned char ch)
		{
			return static_cast<char>(std::tolower(ch));
		});
	return text;
}

int MatchRank(std::string const& name, std::string const& query)
{
	std::string const lower = Lower(name);
	if (lower == query)
	{
		return 0;
	}

	if (lower.compare(0, query.size(), query) == 0)
	{
		return 1;
	}

	if (lower.find(query) != std::string::npos)
	{
		return 2;
	}

	size_t at = 0;
	for (char ch : lower)
	{
		if (at < query.size() && ch == query[at])
		{
			++at;
		}
	}

	return at == query.size() ? 3 : 4;
}

char const* Description(std::string const& name)
{
	static std::map<std::string, char const*> const descriptions = {
		{ "Node", "Base element of the scene tree. Organizes child nodes and their lifecycle." },
		{ "Node3D", "A scene node with position, rotation and scale in three dimensions." },
		{ "Node2D", "A scene node with a two-dimensional transform." },
		{ "Control", "Base control for user interfaces, with layout, drawing and input." },
		{ "Camera3D", "Defines the view and projection used to render a 3D scene." },
		{ "MeshInstance3D", "Displays a mesh resource in the 3D scene." },
		{ "Label", "Displays text using the current theme font and colors." },
		{ "Button", "An interactive control that triggers an action when pressed." },
		{ "LineEdit", "An editable single-line text field." },
		{ "SubViewport", "Renders its scene into a separate viewport texture." },
		{ "DirectionalLight3D", "Illuminates the scene from one direction." },
		{ "VBoxContainer", "Arranges child controls vertically." },
		{ "HBoxContainer", "Arranges child controls horizontally." }
	};
	auto found = descriptions.find(name);
	return found == descriptions.end() ? "" : found->second;
}
}

void CreateNodePopup::SetCreateCallback(std::function<bool(std::string const&, std::string&)> callback)
{
	m_createCallback = std::move(callback);
}

void CreateNodePopup::Open(ObjectID parentID)
{
	m_parentID      = parentID;
	m_selectedClass = "Node";
	m_openRequested = true;
}

void CreateNodePopup::Render(EditorUIContext& context)
{
	m_context = context;
	if (m_popup == nullptr && !m_openRequested)
	{
		return;
	}

	if (m_popup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_popup                = EditorPopupUtils::Create(context.m_rootControl, "Create New Node", content);
		m_popup->SetKeyCallback(
			[this](InputEvent const& event)
			{
				HandleKey(event);
			});
		auto* body = MemNew<HBoxContainer>();
		body->SetVerticalSizeFlags(Control::Size_ExpandFill);
		content->AddNode(body);
		auto* sidebar = MemNew<PanelContainer>();
		sidebar->SetThemeTypeVariation("EditorDialogSidebar");
		sidebar->AddThemeChangedCallback(
			[control = sidebar]()
			{
				control->SetCustomMinimumSize(
					Vector2(static_cast<float>(control->GetThemeConstant("node_sidebar_width", "Editor")), 0.f));
			});
		body->AddNode(sidebar);
		auto* sideScroll = MemNew<ScrollContainer>();
		sidebar->AddNode(sideScroll);
		m_sidebar = MemNew<VBoxContainer>();
		sideScroll->AddNode(m_sidebar);
		auto* main = MemNew<VBoxContainer>();
		main->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		body->AddNode(main);
		auto* searchRow = MemNew<HBoxContainer>();
		main->AddNode(searchRow);
		m_search = MemNew<LineEdit>();
		m_search->SetName("SearchNodeTypes");
		m_search->SetPlaceholder("Search node types");
		m_search->SetMinimumCharacterWidth(1);
		m_search->SetIcon(content->GetThemeIcon("Search", "EditorIcons"));
		m_search->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		searchRow->AddNode(m_search);
		m_search->SetTextChangedCallback(
			[this](String const&)
			{
				RebuildTypes();
			});
		Button* clear = EditorPopupUtils::AddButton(
			searchRow,
			"",
			[this]()
			{
				m_search->SetText("");
				RebuildTypes();
				m_search->GrabFocus();
			});
		clear->SetIcon(content->GetThemeIcon("Close", "EditorIcons"));
		clear->SetThemeTypeVariation("EditorPanelAction");
		m_favorite = EditorPopupUtils::AddButton(
			searchRow,
			"Favorite",
			[this]()
			{
				auto found = std::find(m_favorites.begin(), m_favorites.end(), m_selectedClass);
				if (found == m_favorites.end())
				{
					m_favorites.push_back(m_selectedClass);
				}
				else
				{
					m_favorites.erase(found);
				}

				SaveState();
				RebuildSidebar();
				RefreshSelection();
			});
		m_favorite->SetToggleMode(true);
		EditorPopupUtils::AddLabel(main, "Matches", "EditorStatusLabel");
		auto* scroll = MemNew<ScrollContainer>();
		scroll->SetVerticalSizeFlags(Control::Size_ExpandFill);
		main->AddNode(scroll);
		m_types = MemNew<Tree>();
		m_types->SetName("NodeTypes");
		m_types->SetClipText(true);
		scroll->AddNode(m_types);
		m_types->SetSelectedCallback(
			[this](uint64_t id)
			{
				auto found      = m_typeNames.find(id);
				m_selectedClass = found != m_typeNames.end() ? found->second : "";
				RefreshSelection();
			});
		m_types->SetActivatedCallback(
			[this](uint64_t)
			{
				Confirm();
			});
		auto* info = MemNew<PanelContainer>();
		info->SetThemeTypeVariation("EditorDialogSidebar");
		info->AddThemeChangedCallback(
			[control = info]()
			{
				control->SetCustomMinimumSize(
					Vector2(0.f, static_cast<float>(control->GetThemeConstant("node_description_height", "Editor"))));
			});
		main->AddNode(info);
		m_description = EditorPopupUtils::AddLabel(info, "");
		m_description->SetClipText(true);
		m_description->SetAutowrapMode(AutowrapMode::WordSmart);
		m_error = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		m_error->SetClipText(true);
		auto* buttons = MemNew<HBoxContainer>();
		content->AddNode(buttons);
		m_parentLabel = EditorPopupUtils::AddLabel(buttons, "", "EditorStatusLabel");
		m_parentLabel->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		m_parentLabel->SetVerticalAlignment(VerticalAlignment::Center);
		m_parentLabel->SetClipText(true);
		EditorPopupUtils::AddButton(
			buttons,
			"Cancel",
			[this]()
			{
				m_popup->Close();
			});
		m_createButton = EditorPopupUtils::AddButton(
			buttons,
			"Create",
			[this]()
			{
				Confirm();
			});
		m_createButton->SetThemeTypeVariation("EditorPrimaryButton");
	}

	if (m_openRequested)
	{
		m_openRequested = false;
		LoadState();
		m_search->SetText("");
		m_error->SetText("");
		Node* parent = ObjectDatabase::GetInstance<Node>(m_parentID);
		m_parentLabel->SetText(
			parent != nullptr ? String(std::string("Child of ") + parent->GetName().ToUtf8())
							  : String("Create scene root"));
		RebuildSidebar();
		RebuildTypes();
		EditorPopupUtils::Open(
			m_popup,
			context.m_rootControl,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_height", "Editor"))));
		m_search->GrabFocus();
		m_types->EnsureSelectedVisible();
	}

	if (m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_popup,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_height", "Editor"))));
	}
}

void CreateNodePopup::RebuildTypes()
{
	std::string const               query   = Lower(m_search->GetText().ToUtf8());
	auto                            classes = ClassDatabase::GetRegisteredClasses(true);
	std::map<std::string, uint64_t> keys;
	for (ClassInfo const* info : classes)
	{
		if (info != nullptr && info->m_apiType == ApiType::Runtime
			&& ClassDatabase::IsSubclassOf(info->m_className, "Node"))
		{
			keys[info->m_className] = keys.size() + 1;
		}
	}

	std::set<std::string> visible;
	std::string           best;
	auto                  score = [this, &query](std::string const& name)
	{
		auto recent = std::find(m_recent.begin(), m_recent.end(), name);
		return std::make_tuple(
			MatchRank(name, query),
			name.size(),
			std::find(m_favorites.begin(), m_favorites.end(), name) == m_favorites.end(),
			std::distance(m_recent.begin(), recent),
			name);
	};
	for (auto const& entry : keys)
	{
		if (MatchRank(entry.first, query) == 4)
		{
			continue;
		}

		ClassInfo const* info = ClassDatabase::GetClassInfo(entry.first);
		if (CanCreateClass(info) && (best.empty() || score(entry.first) < score(best)))
		{
			best = entry.first;
		}

		while (info != nullptr && keys.count(info->m_className) != 0)
		{
			visible.insert(info->m_className);
			info = ClassDatabase::GetClassInfo(info->m_parentClassName);
		}
	}

	m_typeNames.clear();
	std::vector<Tree::Item>                              items;
	std::map<std::string, std::vector<ClassInfo const*>> children;
	for (ClassInfo const* info : classes)
	{
		if (info != nullptr && visible.count(info->m_className) != 0)
		{
			children[info->m_parentClassName].push_back(info);
		}
	}

	std::function<void(ClassInfo const*)> append = [&](ClassInfo const* info)
	{
		uint64_t const id = keys.at(info->m_className);
		Tree::Item     item{ id,
							 keys.count(info->m_parentClassName) != 0 ? keys.at(info->m_parentClassName) : 0,
							 info->m_className,
							 true,
							 false,
							 EditorIcons::GetIconTexture(info->m_className, "Node"),
							 EditorIcons::GetIconColor(info->m_className, m_types) };
		item.m_dimmed = !CanCreateClass(info);
		items.push_back(std::move(item));
		m_typeNames[id] = info->m_className;
		for (ClassInfo const* child : children[info->m_className])
		{
			append(child);
		}
	};
	if (visible.count("Node") != 0)
	{
		append(ClassDatabase::GetClassInfo("Node"));
	}

	m_types->SetItems(std::move(items), !query.empty());
	m_selectedClass = query.empty() && visible.count("Node") != 0 ? "Node" : best;
	m_types->Select(keys.count(m_selectedClass) != 0 ? keys.at(m_selectedClass) : 0);
	RefreshSelection();
}

void CreateNodePopup::RefreshSelection()
{
	ClassInfo const* info      = ClassDatabase::GetClassInfo(m_selectedClass);
	bool const       canCreate = CanCreateClass(info);
	m_createButton->SetDisabled(!canCreate);
	m_favorite->SetDisabled(!canCreate);
	m_favorite->SetPressedNoSignal(
		std::find(m_favorites.begin(), m_favorites.end(), m_selectedClass) != m_favorites.end());
	if (info == nullptr)
	{
		m_description->SetText("No matching node types.");
		return;
	}

	m_description->SetText(
		m_selectedClass + "\nInherits " + info->m_parentClassName
		+ (canCreate ? "\n" : "\nBase class. Cannot be created directly.\n") + Description(m_selectedClass));
}

void CreateNodePopup::RebuildSidebar()
{
	EditorPopupUtils::ClearContent(m_sidebar);
	auto section = [this](char const* heading, std::vector<std::string> const& names)
	{
		EditorPopupUtils::AddLabel(m_sidebar, heading, "EditorStatusLabel");
		for (auto const& name : names)
		{
			Button* button = EditorPopupUtils::AddButton(
				m_sidebar,
				name,
				[this, name]()
				{
					m_search->SetText(name);
					RebuildTypes();
					m_search->GrabFocus();
				});
			button->SetIcon(EditorIcons::GetIconTexture(name, "Node"));
			button->SetThemeTypeVariation("EditorToolbarButton");
			button->SetHorizontalAlignment(HorizontalAlignment::Left);
			button->SetClipText(true);
		}

		if (names.empty())
		{
			EditorPopupUtils::AddLabel(m_sidebar, "Nothing yet", "EditorStatusLabel");
		}
	};
	section("FAVORITES", m_favorites);
	section("RECENT", m_recent);
}

void CreateNodePopup::Confirm()
{
	if (!CanCreateClass(ClassDatabase::GetClassInfo(m_selectedClass)) || !m_createCallback)
	{
		return;
	}

	std::string error;
	if (!m_createCallback(m_selectedClass, error))
	{
		m_error->SetText(error);
		return;
	}

	m_recent.erase(std::remove(m_recent.begin(), m_recent.end(), m_selectedClass), m_recent.end());
	m_recent.insert(m_recent.begin(), m_selectedClass);
	if (m_recent.size() > 15)
	{
		m_recent.resize(15);
	}

	SaveState();
	m_popup->Close();
}

void CreateNodePopup::LoadState()
{
	m_favorites.clear();
	m_recent.clear();
	std::string text;
	if (m_context.m_fileSystem == nullptr
		|| !m_context.m_fileSystem->ReadText("res://.ming/Editor/CreateNodeState.json", text))
	{
		return;
	}

	auto state = nlohmann::json::parse(text, nullptr, false);
	if (!state.is_object())
	{
		return;
	}

	auto load = [&state](char const* key, std::vector<std::string>& values, size_t limit)
	{
		auto found = state.find(key);
		if (found == state.end() || !found->is_array())
		{
			return;
		}

		for (auto const& entry : *found)
		{
			if (!entry.is_string())
			{
				continue;
			}

			std::string const name = entry.get<std::string>();
			if (values.size() < limit && CanCreateClass(ClassDatabase::GetClassInfo(name))
				&& std::find(values.begin(), values.end(), name) == values.end())
			{
				values.push_back(name);
			}
		}
	};
	load("favorites", m_favorites, 256);
	load("recent", m_recent, 15);
}

void CreateNodePopup::SaveState()
{
	if (m_context.m_fileSystem != nullptr)
	{
		nlohmann::json state = { { "favorites", m_favorites }, { "recent", m_recent } };
		if (!m_context.m_fileSystem->WriteText("res://.ming/Editor/CreateNodeState.json", state.dump(2)))
		{
			m_error->SetText("Could not save favorites and recent node types.");
		}
	}
}

void CreateNodePopup::HandleKey(InputEvent const& event)
{
	if (event.m_type == InputEventType::Key && event.m_pressed)
	{
		if (event.m_keyCode == GLFW_KEY_ESCAPE)
		{
			m_popup->Close();
			return;
		}

		if (event.m_keyCode == GLFW_KEY_ENTER || event.m_keyCode == GLFW_KEY_KP_ENTER)
		{
			if (auto* button = dynamic_cast<Button*>(m_popup->GetViewport()->GetGuiFocus()))
			{
				button->GuiInput(event);
				return;
			}

			Confirm();
			return;
		}

		if (event.m_keyCode == GLFW_KEY_UP || event.m_keyCode == GLFW_KEY_DOWN || event.m_keyCode == GLFW_KEY_PAGE_UP
			|| event.m_keyCode == GLFW_KEY_PAGE_DOWN)
		{
			InputEvent navigation = event;
			bool const page       = event.m_keyCode == GLFW_KEY_PAGE_UP || event.m_keyCode == GLFW_KEY_PAGE_DOWN;
			navigation.m_keyCode =
				event.m_keyCode == GLFW_KEY_UP || event.m_keyCode == GLFW_KEY_PAGE_UP ? GLFW_KEY_UP : GLFW_KEY_DOWN;
			int const count =
				page ? std::max(
						   1,
						   static_cast<int>(
							   static_cast<Control*>(m_types->GetParent())->GetSize().y / m_types->GetRowHeight()))
					 : 1;
			for (int i = 0; i < count; ++i)
			{
				m_types->GuiInput(navigation);
			}
			return;
		}
	}

	Control* focus = m_popup->GetViewport()->GetGuiFocus();
	if (focus != nullptr && focus != m_popup)
	{
		focus->GuiInput(event);
	}
}

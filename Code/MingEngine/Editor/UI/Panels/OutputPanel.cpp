#include "MingEngine/Editor/UI/Panels/OutputPanel.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

#include <algorithm>
#include <cctype>

OutputPanel::OutputPanel() : EditorPanel("Output")
{
}

void OutputPanel::OnInitialize()
{
	auto* margin = MemNew<MarginContainer>();

	margin->AddThemeChangedCallback(
		[control = margin]()
		{
			control->SetMargins(
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	AddNode(margin);
	m_layout = MemNew<VBoxContainer>();
	m_layout->AddThemeChangedCallback(
		[control = m_layout]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	margin->AddNode(m_layout);
	auto* area = MemNew<PanelContainer>();
	area->SetThemeTypeVariation("EditorLogArea");
	area->SetVerticalSizeFlags(Size_ExpandFill);
	m_layout->AddNode(area);
	auto* scroll = MemNew<ScrollContainer>();
	m_scroll     = scroll;
	scroll->SetThemeTypeVariation("EditorLogScroll");
	area->AddNode(scroll);
	m_lines = MemNew<VBoxContainer>();
	scroll->AddNode(m_lines);
	auto* tools = MemNew<HBoxContainer>();
	tools->AddThemeChangedCallback(
		[control = tools]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	m_layout->AddNode(tools);
	auto* clear = EditorPopupUtils::AddButton(
		tools,
		"",
		[this]()
		{
			ClearMessages();
		});
	clear->SetName("ClearOutput");
	clear->SetIcon(GetThemeIcon("Close", "EditorIcons"));
	clear->SetThemeTypeVariation("EditorPanelAction");
	auto* combine = EditorPopupUtils::AddButton(
		tools,
		"",
		[this]()
		{
			m_combine = !m_combine;
			m_dirty   = true;
		});
	combine->SetName("CombineOutput");
	combine->SetIcon(GetThemeIcon("ItemList", "EditorIcons"));
	combine->SetThemeTypeVariation("EditorPanelAction");
	combine->SetToggleMode(true);
	auto* search = MemNew<LineEdit>();
	search->SetName("SearchOutput");
	search->SetPlaceholder("Filter Messages");
	search->SetMinimumCharacterWidth(1);
	search->SetHorizontalSizeFlags(Size_ExpandFill);
	search->SetIcon(GetThemeIcon("Search", "EditorIcons"));
	search->SetTextChangedCallback(
		[this](String const& text)
		{
			m_searchText = text.ToUtf8();
			std::transform(
				m_searchText.begin(),
				m_searchText.end(),
				m_searchText.begin(),
				[](unsigned char ch)
				{
					return static_cast<char>(std::tolower(ch));
				});
			m_dirty = true;
		});
	tools->AddNode(search);
	Button* clearSearch = EditorPopupUtils::AddButton(
		tools,
		"",
		[this, search]()
		{
			search->SetText("");
			m_searchText.clear();
			m_dirty = true;
			search->GrabFocus();
		});
	clearSearch->SetName("ClearOutputSearch");
	clearSearch->SetIcon(GetThemeIcon("Close", "EditorIcons"));
	clearSearch->SetThemeTypeVariation("EditorPanelAction");
	char const* icons[]  = { "Info", "StatusWarning", "StatusError" };
	char const* colors[] = { "tn", "twarn", "terr" };
	for (int i = 0; i < 3; ++i)
	{
		auto* button = EditorPopupUtils::AddButton(
			tools,
			"0",
			[this, i]()
			{
				m_filters[i] = !m_filters[i];
				m_dirty      = true;
			});
		button->SetName(icons[i]);
		button->SetThemeTypeVariation("EditorOutputFilter");
		button->SetIcon(GetThemeIcon(icons[i], "EditorIcons"));
		button->AddThemeColorOverride("font_color", GetThemeColor(colors[i], "EditorIcons"));
		button->AddThemeColorOverride("font_pressed_color", GetThemeColor(colors[i], "EditorIcons"));
		button->AddThemeColorOverride("font_hover_pressed_color", GetThemeColor(colors[i], "EditorIcons"));
		button->SetToggleMode(true);
		m_typeFilters[i] = button;
	}
}

void OutputPanel::AddMessage(Message message)
{
	m_messages.push_back(std::move(message));
	m_dirty = true;
}

void OutputPanel::ClearMessages()
{
	m_messages.clear();
	m_dirty = true;
}

void OutputPanel::SetCollapsed(bool collapsed)
{
	m_collapsed = collapsed;
	static_cast<Control*>(m_layout->GetParent())->SetVisible(!collapsed);
}

void OutputPanel::RebuildMessages()
{
	bool const atBottom = m_scroll->GetScroll() >= m_scroll->GetContentSize().y - m_scroll->GetPageSize().y - 1.f;
	m_follow            = atBottom;
	m_savedScroll       = m_scroll->GetScroll();
	m_restoreScroll     = true;
	EditorPopupUtils::ClearContent(m_lines);
	Message const* previous    = nullptr;
	int            repetitions = 0;
	Label*         line        = nullptr;
	bool           any         = false;
	for (Message const& message : m_messages)
	{
		bool const adjacent =
			previous != nullptr && previous->m_level == message.m_level && previous->m_text == message.m_text;
		previous          = &message;
		std::string lower = message.m_text.ToUtf8();
		std::transform(
			lower.begin(),
			lower.end(),
			lower.begin(),
			[](unsigned char ch)
			{
				return static_cast<char>(std::tolower(ch));
			});
		if ((!m_filters[static_cast<int>(message.m_level)])
			|| (!m_searchText.empty() && lower.find(m_searchText) == std::string::npos))
		{
			continue;
		}

		char const* variation = message.m_level == Level::Info      ? "EditorLogInfo"
								: message.m_level == Level::Warning ? "EditorLogWarning"
																	: "EditorLogError";
		if (m_combine && adjacent && line != nullptr)
		{
			++repetitions;
			line->SetText(message.m_text + " (" + std::to_string(repetitions) + ")");
		}
		else
		{
			line        = EditorPopupUtils::AddLabel(m_lines, message.m_text, variation);
			repetitions = 1;
		}
		any = true;
	}

	if (!any)
	{
		EditorPopupUtils::AddLabel(m_lines, "No messages", "EditorLogTime");
	}

	m_dirty = false;
}

void OutputPanel::OnRender(EditorUIContext&)
{
	if (m_restoreScroll)
	{
		m_scroll->SetScroll(m_follow ? m_scroll->GetContentSize().y : m_savedScroll);
		m_restoreScroll = false;
	}
	for (int i = 0; i < 3; ++i)
	{
		size_t const count = std::count_if(
			m_messages.begin(),
			m_messages.end(),
			[i](Message const& message)
			{
				return static_cast<int>(message.m_level) == i;
			});
		m_typeFilters[i]->SetText(std::to_string(count));
		m_typeFilters[i]->SetPressedNoSignal(m_filters[i]);
	}
	auto* tabs = dynamic_cast<TabContainer*>(GetParent());
	if (tabs != nullptr)
	{
		Button* output = tabs->GetTabButton(tabs->GetPageIndex(this));
		output->SetText("Output " + std::to_string(m_messages.size()));
	}

	if (m_dirty)
	{
		RebuildMessages();
	}
}

#include "MingEngine/Editor/UI/OutputPanel.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

#include <algorithm>

OutputPanel::OutputPanel() : EditorPanel("Output")
{
}

void OutputPanel::OnInitialize()
{
	auto* margin = MemNew<MarginContainer>();
	float const padding = static_cast<float>(GetThemeConstant("panel_margin", "Editor"));
	margin->SetMargins(padding, padding, padding, padding);
	AddNode(margin);
	m_layout = MemNew<VBoxContainer>();
	m_layout->SetSeparation(padding);
	margin->AddNode(m_layout);
	auto* area = MemNew<PanelContainer>();
	area->SetThemeTypeVariation("EditorLogArea");
	area->SetVerticalSizeFlags(Size_ExpandFill);
	m_layout->AddNode(area);
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetThemeTypeVariation("EditorLogScroll");
	area->AddNode(scroll);
	m_lines = MemNew<VBoxContainer>();
	scroll->AddNode(m_lines);
	auto* tools = MemNew<HBoxContainer>();
	tools->SetSeparation(padding * 0.5f);
	m_layout->AddNode(tools);
	auto* clear = EditorPopupUtils::AddButton(tools, "", [this]() { ClearMessages(); });
	clear->SetName("ClearOutput");
	clear->SetIcon(GetThemeIcon("Close", "EditorIcons"));
	clear->SetThemeTypeVariation("EditorPanelAction");
	auto* combine = EditorPopupUtils::AddButton(tools, "", {});
	combine->SetName("CombineOutput");
	combine->SetIcon(GetThemeIcon("ItemList", "EditorIcons"));
	combine->SetThemeTypeVariation("EditorPanelAction");
	combine->SetDisabled(true);
	auto* search = MemNew<LineEdit>();
	search->SetName("SearchOutput");
	search->SetPlaceholder("Filter Messages");
	search->SetMinimumCharacterWidth(1);
	search->SetHorizontalSizeFlags(Size_ExpandFill);
	search->SetIcon(GetThemeIcon("Search", "EditorIcons"));
	search->SetDisabled(true);
	tools->AddNode(search);
	char const* icons[] = { "Info", "StatusWarning", "StatusError" };
	char const* colors[] = { "tn", "twarn", "terr" };
	for (int i = 0; i < 3; ++i)
	{
		auto* button = EditorPopupUtils::AddButton(tools, "0", [this, i]()
		{
			m_filter = m_filter == i + 1 ? 0 : i + 1;
			m_errorsOnly = false;
			m_dirty = true;
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
	m_errors = MemNew<Button>();
	m_errors->SetThemeTypeVariation("EditorOutputTab");
	m_errors->SetToggleMode(true);
	m_errors->SetPressedCallback([this]() { SetErrorsOnly(true); });
	AddNode(m_errors);
	m_errors->SetVisible(false);
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

void OutputPanel::SetErrorsOnly(bool errorsOnly)
{
	m_errorsOnly = errorsOnly;
	m_dirty = true;
}

void OutputPanel::SetCollapsed(bool collapsed)
{
	m_collapsed = collapsed;
	static_cast<Control*>(m_layout->GetParent())->SetVisible(!collapsed);
}

void OutputPanel::RebuildMessages()
{
	EditorPopupUtils::ClearContent(m_lines);
	int const filter = m_filter;
	bool any = false;
	for (Message const& message : m_messages)
	{
		if ((m_errorsOnly && message.m_level != Level::Error)
			|| (!m_errorsOnly && filter != 0 && filter != static_cast<int>(message.m_level) + 1))
		{
			continue;
		}

		char const* variation = message.m_level == Level::Info ? "EditorLogInfo"
			: message.m_level == Level::Warning ? "EditorLogWarning" : "EditorLogError";
		EditorPopupUtils::AddLabel(m_lines, message.m_text, variation);
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
	for (int i = 0; i < 3; ++i)
	{
		size_t const count = std::count_if(m_messages.begin(), m_messages.end(), [i](Message const& message)
		{
			return static_cast<int>(message.m_level) == i;
		});
		m_typeFilters[i]->SetText(std::to_string(count));
		m_typeFilters[i]->SetPressedNoSignal(m_errorsOnly ? i == 2 : m_filter == 0 || m_filter == i + 1);
	}
	auto* tabs = dynamic_cast<TabContainer*>(GetParent());
	if (tabs != nullptr)
	{
		if (m_errors->GetParent() != tabs->GetTabExtras())
		{
			m_errors->Reparent(tabs->GetTabExtras(), false);
		}

		m_errors->SetVisible(tabs->GetCurrentPage() == this);
		size_t const errors = std::count_if(m_messages.begin(), m_messages.end(), [](Message const& message) { return message.m_level == Level::Error; });
		m_errors->SetText("Errors " + std::to_string(errors));
		m_errors->SetPressedNoSignal(m_errorsOnly);
		Button* output = tabs->GetTabButton(tabs->GetPageIndex(this));
		output->SetText("Output " + std::to_string(m_messages.size()));
		output->SetPressedNoSignal(!m_errorsOnly && tabs->GetCurrentPage() == this);
		output->SetPressedCallback([this, tabs]()
		{
			tabs->SetCurrentTab(tabs->GetPageIndex(this));
			SetErrorsOnly(false);
		});
	}
	else
	{
		m_errors->SetVisible(false);
	}

	if (m_dirty)
	{
		RebuildMessages();
	}
}

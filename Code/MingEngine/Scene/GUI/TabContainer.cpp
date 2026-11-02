#include "MingEngine/Scene/GUI/TabContainer.hpp"

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"

#include <algorithm>
#include "ThirdParty/GLFW/glfw3.h"

namespace
{
void FinishPageEditing(Node* node)
{
	if (auto* edit = dynamic_cast<LineEdit*>(node))
	{
		edit->EndEditing(LineEdit::EditEndReason::FocusLost);
	}

	for (Node* child : node->GetChildren())
	{
		FinishPageEditing(child);
	}
}
}

TabContainer::TabContainer()
{
	SetClipContents(true);
	m_titleActions = MemNew<HBoxContainer>();
	m_tabExtras = MemNew<HBoxContainer>();
	AddNode(m_titleActions);
	AddNode(m_tabExtras);
}

void TabContainer::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Font, TabContainer, font);
	BIND_THEME_ITEM(Theme::DataType::FontSize, TabContainer, font_size);
	BIND_THEME_ITEM(Theme::DataType::Constant, TabContainer, tab_height);
	BIND_THEME_ITEM(Theme::DataType::Constant, TabContainer, tab_minimum_width);
	BIND_THEME_ITEM(Theme::DataType::Constant, TabContainer, tab_separation);
	BIND_THEME_ITEM(Theme::DataType::Constant, TabContainer, page_separation);
	BIND_THEME_ITEM(Theme::DataType::Constant, TabContainer, drop_mark_width);
	BIND_THEME_ITEM(Theme::DataType::Color, TabContainer, font_color);
	BIND_THEME_ITEM(Theme::DataType::Color, TabContainer, font_hover_color);
	BIND_THEME_ITEM(Theme::DataType::Color, TabContainer, font_selected_color);
	BIND_THEME_ITEM(Theme::DataType::Color, TabContainer, font_selected_hover_color);
	BIND_THEME_ITEM(Theme::DataType::Color, TabContainer, font_disabled_color);
	BIND_THEME_ITEM(Theme::DataType::Color, TabContainer, drop_mark_color);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, TabContainer, panel);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, TabContainer, tab_normal);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, TabContainer, tab_hover);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, TabContainer, tab_selected);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, TabContainer, tab_selected_hover);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, TabContainer, tab_disabled);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, TabContainer, tab_focus);
}

void TabContainer::ApplyTabTheme(Button* button)
{
	button->SetFont(m_themeCache.font);
	button->SetFontSize(m_themeCache.font_size);
	button->AddThemeConstantOverride("minimum_height", std::max(0, m_themeCache.tab_height));
	button->AddThemeStyleBoxOverride("normal", m_themeCache.tab_normal);
	button->AddThemeStyleBoxOverride("hover", m_themeCache.tab_hover);
	button->AddThemeStyleBoxOverride("pressed", m_themeCache.tab_selected);
	button->AddThemeStyleBoxOverride("hover_pressed", m_themeCache.tab_selected_hover);
	button->AddThemeStyleBoxOverride("disabled", m_themeCache.tab_disabled);
	button->AddThemeStyleBoxOverride("focus", m_themeCache.tab_focus);
	button->AddThemeColorOverride("font_color", m_themeCache.font_color);
	button->AddThemeColorOverride("font_hover_color", m_themeCache.font_hover_color);
	button->AddThemeColorOverride("font_pressed_color", m_themeCache.font_selected_color);
	button->AddThemeColorOverride("font_hover_pressed_color", m_themeCache.font_selected_hover_color);
	button->AddThemeColorOverride("font_disabled_color", m_themeCache.font_disabled_color);
}

float TabContainer::GetTabWidth(Button const* button) const
{
	float textWidth = 0.f;
	if (m_themeCache.font.IsValid() && g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		textWidth = g_engine->m_textServer->GetTextLine(m_themeCache.font->GetFontRID(), button->GetText(),
			static_cast<float>(m_themeCache.font_size)).m_width;
	}

	float const padding = m_themeCache.tab_normal.IsValid() ? m_themeCache.tab_normal->GetMinimumSize().x : 0.f;
	return std::max(static_cast<float>(std::max(0, m_themeCache.tab_minimum_width)), textWidth + padding);
}

void TabContainer::OnThemeChanged()
{
	for (Button* button : m_buttons)
	{
		ApplyTabTheme(button);
	}

	MarkDirty();
}

AABB2 TabContainer::GetTabBarRect() const
{
	float height = static_cast<float>(std::max(0, m_themeCache.tab_height));
	for (Button* button : m_buttons)
	{
		height = std::max(height, button->GetCombinedMinimumSize().y);
	}

	float const top = m_tabsAtBottom ? std::max(0.f, GetSize().y - height) : 0.f;
	return AABB2(0.f, top, std::max(0.f, GetSize().x - m_titleActions->GetCombinedMinimumSize().x - m_tabExtras->GetCombinedMinimumSize().x), top + height);
}

AABB2 TabContainer::GetPageRect() const
{
	float const top = std::min(GetSize().y, GetTabBarRect().m_maxs.y + static_cast<float>(std::max(0, m_themeCache.page_separation)));
	float const bottom = std::max(0.f, GetTabBarRect().m_mins.y - static_cast<float>(std::max(0, m_themeCache.page_separation)));
	AABB2 const area(0.f, m_tabsAtBottom ? 0.f : top, GetSize().x, m_tabsAtBottom ? bottom : GetSize().y);
	return m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentRect(area) : area;
}

AABB2 TabContainer::GetTabInsertRect(int index) const
{
	if (index < 0 || index > GetPageCount())
	{
		return AABB2::Zero;
	}

	float const available = GetTabBarRect().GetWidth();
	float const width = std::min(available, static_cast<float>(std::max(1, m_themeCache.drop_mark_width)));
	float const edge = index < GetPageCount() ? m_buttons[index]->GetLocalPosition().x
		: (m_buttons.empty() ? 0.f : m_buttons.back()->GetLocalPosition().x + m_buttons.back()->GetSize().x);
	float const x = std::clamp(edge, 0.f, std::max(0.f, available - width));
	return AABB2(x, GetTabBarRect().m_mins.y, x + width, GetTabBarRect().m_maxs.y);
}

Vector2 TabContainer::GetInternalMinimumSize() const
{
	Vector2 minimum = m_themeCache.panel.IsValid() ? m_themeCache.panel->GetMinimumSize() : Vector2::Zero;
	Vector2 pageMinimum;
	for (Control* page : m_pages)
	{
		Vector2 const size = page->GetCombinedMinimumSize();
		pageMinimum.x = std::max(pageMinimum.x, size.x);
		pageMinimum.y = std::max(pageMinimum.y, size.y);
	}

	minimum = minimum + pageMinimum;
	for (Button* button : m_buttons)
	{
		minimum.x = std::max({ minimum.x, button->GetCombinedMinimumSize().x,
			static_cast<float>(std::max(0, m_themeCache.tab_minimum_width)) });
	}

	minimum.x = std::max(minimum.x, m_titleActions->GetCombinedMinimumSize().x + 60.f);
	minimum.y += GetTabBarRect().GetHeight() + static_cast<float>(std::max(0, m_themeCache.page_separation));
	return minimum;
}

void TabContainer::AddPage(String const& title, Control* page)
{
	InsertPage(title, page, GetPageCount());
}

void TabContainer::InsertPage(String const& title, Control* page, int index)
{
	if (page == nullptr)
	{
		return;
	}

	if (auto* previous = dynamic_cast<TabContainer*>(page->GetParent()))
	{
		int const oldIndex = previous->GetPageIndex(page);
		if (oldIndex >= 0)
		{
			previous->RemovePage(oldIndex);
		}
	}

	index = std::clamp(index, 0, GetPageCount());
	Control* const current = GetCurrentPage();
	auto*     button = MemNew<Button>();
	button->SetText(title);
	button->SetClipText(true);
	button->SetToggleMode(true);
	button->SetPressedCallback(
		[this, page]()
		{
			SetCurrentTab(GetPageIndex(page));
		});
	button->SetDragCallbacks(
		[this, page, title](Vector2 const&, GuiDragData& data)
		{
			if (m_drag)
			{
				return m_drag(page, data);
			}

			data = { "tab", page->GetObjectID(), title };
			return m_rearrangeGroup >= 0;
		},
		[this, button](Vector2 const& position, GuiDragData const& data)
		{
			return CanDropData(position + button->GetLocalPosition(), data);
		},
		[this, button](Vector2 const& position, GuiDragData const& data)
		{
			DropData(position + button->GetLocalPosition(), data);
		});
	button->SetDragHoverCallback(
		[this, page, button](Vector2 const& position, GuiDragData const& data, bool accepted)
		{
			HoverTab(page, position, data);
			SetDropInsertIndex(accepted ? GetTabInsertIndex(position + button->GetLocalPosition()) : -1);
		});
	AddNode(button);
	AddNode(page);
	m_buttons.insert(m_buttons.begin() + index, button);
	m_pages.insert(m_pages.begin() + index, page);
	ApplyTabTheme(button);
	m_current = GetPageIndex(current);
	page->SetVisible(false);
	if (m_current < 0)
	{
		SetCurrentTab(index);
	}

	MarkDirty();
}

Control* TabContainer::GetPage(int index) const
{
	return index >= 0 && index < GetPageCount() ? m_pages[index] : nullptr;
}

Button* TabContainer::GetTabButton(int index) const
{
	return index >= 0 && index < GetPageCount() ? m_buttons[index] : nullptr;
}

int TabContainer::GetPageIndex(Control const* page) const
{
	auto const it = std::find(m_pages.begin(), m_pages.end(), page);
	return it != m_pages.end() ? static_cast<int>(it - m_pages.begin()) : -1;
}

String TabContainer::GetPageTitle(int index) const
{
	return GetTabButton(index) != nullptr ? GetTabButton(index)->GetText() : String();
}

int TabContainer::GetTabInsertIndex(Vector2 const& position) const
{
	if (position.x < 0.f || position.x >= GetTabBarRect().m_maxs.x)
	{
		return -1;
	}

	for (int i = 0; i < GetPageCount(); ++i)
	{
		if (position.x < m_buttons[i]->GetLocalPosition().x + m_buttons[i]->GetSize().x * 0.5f)
		{
			return i;
		}
	}

	return GetPageCount();
}

Control* TabContainer::RemovePage(int index)
{
	Control* page = GetPage(index);
	if (page == nullptr)
	{
		return nullptr;
	}

	Control* current = GetCurrentPage();
	FinishPageEditing(page);
	if (Viewport* viewport = GetViewport())
	{
		viewport->CancelGuiInteraction(page);
	}

	page->SetVisible(false);
	m_buttons[index]->SetVisible(false);
	m_buttons[index]->QueueFree();
	m_buttons.erase(m_buttons.begin() + index);
	m_pages.erase(m_pages.begin() + index);
	m_current = -1;
	if (!m_pages.empty())
	{
		int const next = current == page ? std::min(index, GetPageCount() - 1) : GetPageIndex(current);
		SetCurrentTab(next);
	}

	MarkDirty();
	return page;
}

void TabContainer::MovePage(int from, int to)
{
	if (GetPage(from) == nullptr)
	{
		return;
	}

	to = std::clamp(to, 0, GetPageCount() - 1);
	Control* const current = GetCurrentPage();
	Control* const page = m_pages[from];
	Button* const button = m_buttons[from];
	m_pages.erase(m_pages.begin() + from);
	m_buttons.erase(m_buttons.begin() + from);
	m_pages.insert(m_pages.begin() + to, page);
	m_buttons.insert(m_buttons.begin() + to, button);
	m_current = GetPageIndex(current);
	MarkDirty();
}

bool TabContainer::CanDropData(Vector2 const& position, GuiDragData const& data) const
{
	if (position.y >= GetTabBarRect().m_mins.y && position.y < GetTabBarRect().m_maxs.y
		&& GetTabInsertIndex(position) < 0)
	{
		return false;
	}

	if (Control::CanDropData(position, data))
	{
		return true;
	}

	Control* page = ObjectDatabase::GetInstance<Control>(data.m_objectID);
	auto* from = page != nullptr ? dynamic_cast<TabContainer*>(page->GetParent()) : nullptr;
	return data.m_type == String("tab") && from != nullptr && from->GetPageIndex(page) >= 0
		&& m_rearrangeGroup >= 0 && from->m_rearrangeGroup == m_rearrangeGroup;
}

void TabContainer::DropData(Vector2 const& position, GuiDragData const& data)
{
	if (data.m_type != String("tab"))
	{
		Control::DropData(position, data);
		return;
	}

	if (!CanDropData(position, data))
	{
		return;
	}

	Control* page = ObjectDatabase::GetInstance<Control>(data.m_objectID);
	auto* from = dynamic_cast<TabContainer*>(page->GetParent());
	int const old = from->GetPageIndex(page);
	int index = GetTabBarRect().IsPointInside(position) ? GetTabInsertIndex(position) : GetPageCount();
	if (from == this)
	{
		MovePage(old, index > old ? index - 1 : index);
	}
	else
	{
		InsertPage(from->GetPageTitle(old), page, index);
	}

	SetCurrentTab(GetPageIndex(page));
}

void TabContainer::SetDropInsertIndex(int index)
{
	if (m_dropIndex != index)
	{
		m_dropIndex = index;
		QueueRedraw();
	}
}

void TabContainer::DragHover(Vector2 const& position, GuiDragData const& data, bool accepted)
{
	SetDropInsertIndex(accepted && position.y >= 0.f && GetTabBarRect().IsPointInside(position) ? GetTabInsertIndex(position) : -1);
	Control::DragHover(position, data, accepted);
}

void TabContainer::HoverTab(Control* page, Vector2 const& position, GuiDragData const& data)
{
	if (position.x < 0.f || data.m_type.IsEmpty() || data.m_type == String("tab") || data.m_type == String("dock"))
	{
		m_hoverPage = ObjectID::Invalid;
		return;
	}

	if (m_hoverPage != page->GetObjectID())
	{
		m_hoverPage = page->GetObjectID();
		m_hoverStarted = glfwGetTime();
	}
	else if (glfwGetTime() - m_hoverStarted >= 0.5)
	{
		SetCurrentTab(GetPageIndex(page));
	}
}

Control* TabContainer::GetCurrentPage() const
{
	return m_current >= 0 ? m_pages[m_current] : nullptr;
}

void TabContainer::SetCurrentTab(int index)
{
	if (index < 0 || index >= static_cast<int>(m_pages.size()))
	{
		return;
	}

	if (index == m_current)
	{
		m_buttons[index]->SetPressedNoSignal(true);
		return;
	}

	if (Control* previous = GetCurrentPage())
	{
		FinishPageEditing(previous);
		if (Viewport* viewport = GetViewport())
		{
			if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
			{
				popup->Close(false);
			}

			viewport->CancelGuiInteraction(previous, true, false);
		}

		previous->SetVisible(false, false);
	}

	m_current = index;
	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		m_buttons[i]->SetPressedNoSignal(static_cast<int>(i) == index);
	}

	m_pages[index]->SetVisible(true);
	MarkDirty();
	if (m_changed)
	{
		m_changed(index);
	}
}

void TabContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_UpdateLayout:
	{
		float const available = GetTabBarRect().GetWidth();
		float const actionWidth = m_titleActions->GetCombinedMinimumSize().x;
		m_titleActions->SetPosition(Vector2(GetSize().x - actionWidth, GetTabBarRect().m_mins.y));
		m_titleActions->UpdateSize(Vector2(actionWidth, GetTabBarRect().GetHeight()));
		float total = 0.f;
		for (Button* button : m_buttons)
		{
			total += GetTabWidth(button)
				+ static_cast<float>(std::max(0, m_themeCache.tab_separation));
		}

		float const ratio = total > 0.f ? std::min(1.f, available / total) : 1.f;
		float x = 0.f;
		for (Button* button : m_buttons)
		{
			float const width = GetTabWidth(button) * ratio;
			button->SetPosition(Vector2(x, GetTabBarRect().m_mins.y));
			button->UpdateSize(Vector2(width, GetTabBarRect().GetHeight()));
			x += width + static_cast<float>(std::max(0, m_themeCache.tab_separation)) * ratio;
		}

		if (Control* page = GetCurrentPage())
		{
			AABB2 const rect = GetPageRect();
			page->SetPosition(rect.m_mins);
			page->UpdateSize(rect.GetDimensions());
		}

		m_tabExtras->SetPosition(Vector2(x, GetTabBarRect().m_mins.y));
		m_tabExtras->UpdateSize(Vector2(m_tabExtras->GetCombinedMinimumSize().x, GetTabBarRect().GetHeight()));
		break;
	}
	case Notification_Draw:
	{
		DrawThemeStyleBox(m_themeCache.tab_normal, AABB2(0.f, GetTabBarRect().m_mins.y, GetSize().x, GetTabBarRect().m_maxs.y));
		DrawThemeStyleBox(m_themeCache.panel, m_tabsAtBottom
			? AABB2(0.f, 0.f, GetSize().x, GetTabBarRect().m_mins.y)
			: AABB2(0.f, GetTabBarRect().m_maxs.y, GetSize().x, GetSize().y));
		if (m_dropIndex >= 0)
		{
			DrawRect(GetTabInsertRect(m_dropIndex), m_themeCache.drop_mark_color);
		}
		break;
	}
	}
}

void TabContainer::SetTabsAtBottom(bool enabled)
{
	if (m_tabsAtBottom != enabled)
	{
		m_tabsAtBottom = enabled;
		MarkDirty();
		QueueRedraw();
	}
}

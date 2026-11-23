#include "MingEngine/Scene/GUI/Tree.hpp"

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"

#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

Tree::Tree()
{
	SetClipContents(true);
	SetFocusMode(FocusMode::All);
	SetMouseFilter(MouseFilter::Pass);
	m_edit = MemNew<LineEdit>();
	m_edit->SetVisible(false);
	m_edit->SetMinimumCharacterWidth(1);
	AddNode(m_edit);
	m_edit->SetEditingEndedCallback(
		[this](String const& text, LineEdit::EditEndReason reason)
		{
			uint64_t const id = m_editing;
			m_editing         = 0;
			m_edit->SetVisible(false);
			for (Label* label : m_labels)
			{
				label->SetVisible(true);
			}
			if (id != 0 && reason != LineEdit::EditEndReason::Cancelled
				&& reason != LineEdit::EditEndReason::Interrupted && !text.IsEmpty() && m_renamedCallback)
			{
				m_renamedCallback(id, text);
			}
		});
}

void Tree::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Font, Tree, font);
	BIND_THEME_ITEM(Theme::DataType::FontSize, Tree, font_size);
	BIND_THEME_ITEM(Theme::DataType::Constant, Tree, row_height);
	BIND_THEME_ITEM(Theme::DataType::Constant, Tree, indent);
	BIND_THEME_ITEM(Theme::DataType::Constant, Tree, icon_size);
	BIND_THEME_ITEM(Theme::DataType::Constant, Tree, fold_size);
	BIND_THEME_ITEM(Theme::DataType::Constant, Tree, icon_separation);
	BIND_THEME_ITEM(Theme::DataType::Color, Tree, font_color);
	BIND_THEME_ITEM(Theme::DataType::Color, Tree, font_selected_color);
	BIND_THEME_ITEM(Theme::DataType::Color, Tree, guide_color);
	BIND_THEME_ITEM(Theme::DataType::Constant, Tree, guide_width);
	BIND_THEME_ITEM(Theme::DataType::Constant, Tree, children_guide_width);
	BIND_THEME_ITEM(Theme::DataType::Color, Tree, children_guide_color);
	BIND_THEME_ITEM(Theme::DataType::Icon, Tree, arrow_collapsed);
	BIND_THEME_ITEM(Theme::DataType::Icon, Tree, arrow_expanded);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, panel);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, hover);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, selected);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, focus);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, drop_mark);
	ThemeDatabase::BindThemeItem(Theme::DataType::Constant, GetStaticClassName(), "scrollbar_width", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.scrollbar_width = control->GetThemeConstant("scrollbar_width", "ScrollContainer");
	});

	ThemeDatabase::BindThemeItem(Theme::DataType::Constant, GetStaticClassName(), "grabber_minimum_size", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.grabber_minimum_size = control->GetThemeConstant("grabber_minimum_size", "ScrollContainer");
	});

	ThemeDatabase::BindThemeItem(Theme::DataType::Constant, GetStaticClassName(), "grabber_margin", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.grabber_margin = control->GetThemeConstant("grabber_margin", "ScrollContainer");
	});

	ThemeDatabase::BindThemeItem(Theme::DataType::Constant, GetStaticClassName(), "scroll_step", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.scroll_step = control->GetThemeConstant("scroll_step", "ScrollContainer");
	});

	ThemeDatabase::BindThemeItem(Theme::DataType::StyleBox, GetStaticClassName(), "track", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.track = control->GetThemeStyleBox("track", "ScrollContainer");
	});

	ThemeDatabase::BindThemeItem(Theme::DataType::StyleBox, GetStaticClassName(), "grabber", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.grabber = control->GetThemeStyleBox("grabber", "ScrollContainer");
	});

	ThemeDatabase::BindThemeItem(Theme::DataType::StyleBox, GetStaticClassName(), "grabber_hover", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.grabber_hover = control->GetThemeStyleBox("grabber_hover", "ScrollContainer");
	});

	ThemeDatabase::BindThemeItem(Theme::DataType::StyleBox, GetStaticClassName(), "grabber_pressed", [](Control* control)
	{
		static_cast<Tree*>(control)->m_themeCache.grabber_pressed = control->GetThemeStyleBox("grabber_pressed", "ScrollContainer");
	});

}

void Tree::SetFont(Ref<FontResource> const& font)
{
	AddThemeFontOverride("font", font);
}

void Tree::OnThemeChanged()
{
	if (m_dragAxis >= 0
		&& (m_dragScrollbarWidth != m_themeCache.scrollbar_width
			|| m_dragMinimumSize != m_themeCache.grabber_minimum_size || m_dragMargin != m_themeCache.grabber_margin))
	{
		m_dragAxis = -1;
	}

	UpdateRowLayout();
}

float Tree::GetRowHeight() const
{
	float height = static_cast<float>(std::max(1, m_themeCache.font_size));
	if (m_themeCache.font.IsValid() && m_themeCache.font->GetFontRID().IsValid() && g_engine != nullptr
		&& g_engine->m_textServer != nullptr)
	{
		auto const line = g_engine->m_textServer->GetTextLine(
			m_themeCache.font->GetFontRID(),
			"W",
			static_cast<float>(m_themeCache.font_size));
		height = std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent);
	}

	return std::max(
		{ 1.f,
		  height,
		  static_cast<float>(m_themeCache.row_height),
		  m_edit != nullptr ? m_edit->GetCombinedMinimumSize().y : 0.f });
}

AABB2 Tree::GetRowRect(size_t row) const
{
	float const top =
		(m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Top) : 0.f)
		+ row * GetRowHeight() - m_scroll;
	return AABB2(0.f, top, m_pageSize.x, top + GetRowHeight());
}

AABB2 Tree::GetItemRect(uint64_t id) const
{
	for (size_t row = 0; row < m_rows.size(); ++row)
	{
		if (m_items[m_rows[row]].m_ID == id)
		{
			return GetRowRect(row);
		}
	}

	return AABB2::Zero;
}

int Tree::GetRowAtPosition(Vector2 const& position) const
{
	float const top =
		m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Top) : 0.f;
	if (!AABB2(Vector2::Zero, m_pageSize).IsPointInside(position) || position.y + m_scroll < top)
	{
		return -1;
	}

	int const row = static_cast<int>((position.y + m_scroll - top) / GetRowHeight());
	return row < static_cast<int>(m_rows.size()) ? row : -1;
}

float Tree::GetFoldWidth(size_t row) const
{
	if (m_themeCache.arrow_collapsed.IsValid() && m_themeCache.arrow_expanded.IsValid())
	{
		return static_cast<float>(std::max(0, m_themeCache.fold_size));
	}

	String const prefix(
		HasChildren(m_items[m_rows[row]].m_ID) ? (IsCollapsed(m_items[m_rows[row]].m_ID) && !m_filtering ? "> " : "v ")
											   : "  ");
	if (m_themeCache.font.IsValid() && m_themeCache.font->GetFontRID().IsValid() && g_engine != nullptr
		&& g_engine->m_textServer != nullptr)
	{
		return g_engine->m_textServer
			->GetTextLine(m_themeCache.font->GetFontRID(), prefix, static_cast<float>(m_themeCache.font_size))
			.m_width;
	}

	return static_cast<float>(std::max(1, m_themeCache.font_size));
}

float Tree::GetRowTextX(size_t row) const
{
	float const left =
		m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Left) : 0.f;
	float const iconWidth = m_items[m_rows[row]].m_icon.IsValid()
								? static_cast<float>(m_themeCache.icon_size + m_themeCache.icon_separation)
								: 0.f;
	return left - m_horizontalScroll + m_depths[row] * static_cast<float>(std::max(0, m_themeCache.indent)) + GetFoldWidth(row) + iconWidth;
}

float Tree::GetRowTextEnd(size_t row) const
{
	float const right =
		m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Right) : 0.f;
	return std::max(m_pageSize.x, m_contentSize.x) - m_horizontalScroll - right
		   - m_items[m_rows[row]].m_actions.size() * (m_themeCache.icon_size + m_themeCache.icon_separation);
}

AABB2 Tree::GetItemActionRect(uint64_t id, int action) const
{
	for (size_t row = 0; row < m_rows.size(); ++row)
	{
		auto const& actions = m_items[m_rows[row]].m_actions;
		if (m_items[m_rows[row]].m_ID == id && action >= 0 && action < static_cast<int>(actions.size()))
		{
			float const side = static_cast<float>(m_themeCache.icon_size);
			float const right =
				m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Right) : 0.f;
			float const x = std::max(m_pageSize.x, m_contentSize.x) - m_horizontalScroll - right - side
							- (static_cast<int>(actions.size()) - action - 1) * (side + m_themeCache.icon_separation);
			float const y = GetRowRect(row).GetCenter().y - side * 0.5f;
			return AABB2(x, y, x + side, y + side);
		}
	}

	return AABB2::Zero;
}

void Tree::UpdateRowLayout()
{
	if (m_edit == nullptr || m_updatingRowLayout)
	{
		return;
	}

	m_updatingRowLayout = true;
	m_edit->SetFont(m_themeCache.font);
	m_edit->SetFontSize(m_themeCache.font_size);
	float const left =
		m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Left) : 0.f;
	float const right =
		m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Right) : 0.f;
	float width = std::max(1.f, left + right);
	for (size_t row = 0; row < m_rows.size(); ++row)
	{
		Item const& item         = m_items[m_rows[row]];
		Label*      label        = m_labels[row];
		bool const  texturedFold = m_themeCache.arrow_collapsed.IsValid() && m_themeCache.arrow_expanded.IsValid();
		label->SetText(
			texturedFold
				? item.m_text
				: String(HasChildren(item.m_ID) ? (IsCollapsed(item.m_ID) && !m_filtering ? "> " : "v ") : "  ")
					  + item.m_text);
		label->SetFont(m_themeCache.font);
		label->SetFontSize(m_themeCache.font_size);
		label->SetFontColor(
			item.m_dimmed ? m_themeCache.guide_color
						  : (item.m_ID == m_selected ? m_themeCache.font_selected_color : m_themeCache.font_color));
		label->SetClipText(m_clipText);
		width = std::max(
			width,
			GetRowTextX(row) + m_horizontalScroll - (texturedFold ? 0.f : GetFoldWidth(row))
				+ label->GetCombinedMinimumSize().x + right
				+ item.m_actions.size() * (m_themeCache.icon_size + m_themeCache.icon_separation));
	}

	m_contentWidth = width;
	UpdateScrollRange();
	for (size_t row = 0; row < m_rows.size(); ++row)
	{
		Item const& item = m_items[m_rows[row]];
		Label* label = m_labels[row];
		bool const texturedFold = m_themeCache.arrow_collapsed.IsValid() && m_themeCache.arrow_expanded.IsValid();
		AABB2 const rect = GetRowRect(row);
		label->SetPosition(Vector2(GetRowTextX(row) - (texturedFold ? 0.f : GetFoldWidth(row)), rect.m_mins.y));
		label->UpdateSize(Vector2(
			m_clipText ? std::max(0.f, GetRowTextEnd(row) - label->GetLocalPosition().x)
				: label->GetCombinedMinimumSize().x,
			rect.GetHeight()));
		if (item.m_ID == m_editing)
		{
			float const x = GetRowTextX(row) - m_edit->GetContentRect().m_mins.x;
			m_edit->SetPosition(Vector2(x, rect.m_mins.y));
			m_edit->UpdateSize(Vector2(std::max(1.f, GetRowTextEnd(row) - x), rect.GetHeight()));
		}
	}

	m_updatingRowLayout = false;

	QueueRedraw();
}

void Tree::SetItems(std::vector<Item> items, bool filtering)
{
	bool same = m_filtering == filtering && items.size() == m_items.size();
	for (size_t i = 0; same && i < items.size(); ++i)
	{
		same = items[i].m_ID == m_items[i].m_ID && items[i].m_parentID == m_items[i].m_parentID
			   && items[i].m_text == m_items[i].m_text && items[i].m_visible == m_items[i].m_visible
			   && items[i].m_editable == m_items[i].m_editable && items[i].m_icon == m_items[i].m_icon
			   && items[i].m_iconColor == m_items[i].m_iconColor && items[i].m_dimmed == m_items[i].m_dimmed
			   && items[i].m_actions.size() == m_items[i].m_actions.size();
		for (size_t action = 0; same && action < items[i].m_actions.size(); ++action)
		{
			auto const& a = items[i].m_actions[action];
			auto const& b = m_items[i].m_actions[action];
			same          = a.m_icon == b.m_icon && a.m_color == b.m_color && a.m_enabled == b.m_enabled;
		}
	}

	if (same)
	{
		return;
	}

	m_edit->EndEditing(LineEdit::EditEndReason::Interrupted);
	m_items     = std::move(items);
	m_filtering = filtering;
	std::unordered_set<uint64_t> existing;
	for (Item const& item : m_items)
	{
		existing.insert(item.m_ID);
	}

	for (auto it = m_collapsed.begin(); it != m_collapsed.end();)
	{
		if (existing.count(*it) == 0)
		{
			it = m_collapsed.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (existing.count(m_selected) == 0)
	{
		m_selected = 0;
	}

	RebuildRows();
}

bool Tree::HasChildren(uint64_t id) const
{
	return std::any_of(
		m_items.begin(),
		m_items.end(),
		[id](Item const& item)
		{
			return item.m_parentID == id && item.m_visible;
		});
}

void Tree::RebuildRows()
{
	m_updatingRowLayout = true;
	for (Label* label : m_labels)
	{
		label->SetVisible(false);
		label->QueueFree();
	}

	m_labels.clear();
	m_rows.clear();
	m_depths.clear();
	m_contentWidth = 32.f;
	std::unordered_map<uint64_t, int>  depths;
	std::unordered_map<uint64_t, bool> shown;
	for (size_t i = 0; i < m_items.size(); ++i)
	{
		Item const& item  = m_items[i];
		int const   depth = item.m_parentID == 0 ? 0 : depths[item.m_parentID] + 1;
		depths[item.m_ID] = depth;
		bool const visible =
			item.m_visible
			&& (item.m_parentID == 0 || (shown[item.m_parentID] && (m_filtering || !IsCollapsed(item.m_parentID))));
		shown[item.m_ID] = visible;
		if (!visible)
		{
			continue;
		}

		auto* label = MemNew<Label>();
		label->SetVerticalAlignment(VerticalAlignment::Center);
		label->SetText(
			String(HasChildren(item.m_ID) ? (IsCollapsed(item.m_ID) && !m_filtering ? "> " : "v ") : "  ")
			+ item.m_text);
		AddNode(label);
		m_labels.push_back(label);
		m_rows.push_back(i);
		m_depths.push_back(depth);
	}

	m_updatingRowLayout = false;
	UpdateRowLayout();
	PropagateMinimumSizeChanged();
	QueueRedraw();
}

Vector2 Tree::GetInternalMinimumSize() const
{
	Vector2 const margins = m_themeCache.panel.IsValid() ? m_themeCache.panel->GetMinimumSize() : Vector2::Zero;
	return Vector2(32.f, 24.f) + margins;
}

void Tree::SetCollapsed(uint64_t id, bool collapsed)
{
	if (collapsed)
	{
		m_collapsed.insert(id);
	}
	else
	{
		m_collapsed.erase(id);
	}

	RebuildRows();
}

void Tree::Select(uint64_t id, bool notify)
{
	if (m_selected == id)
	{
		return;
	}

	m_edit->EndEditing(LineEdit::EditEndReason::FocusLost);
	m_selected = id;
	if (!m_filtering)
	{
		uint64_t parent = id;
		while (parent != 0)
		{
			auto it = std::find_if(
				m_items.begin(),
				m_items.end(),
				[parent](Item const& item)
				{
					return item.m_ID == parent;
				});
			parent = it != m_items.end() ? it->m_parentID : 0;
			m_collapsed.erase(parent);
		}

		RebuildRows();
	}

	UpdateRowLayout();
	EnsureSelectedVisible();
	QueueRedraw();
	if (notify && m_selectedCallback)
	{
		m_selectedCallback(id);
	}
}

void Tree::EnsureSelectedVisible()
{
	for (size_t row = 0; row < m_rows.size(); ++row)
	{
		if (m_items[m_rows[row]].m_ID == m_selected)
		{
			AABB2 const rect = GetRowRect(row);
			if (rect.m_mins.y < 0.f || rect.GetHeight() > m_pageSize.y)
			{
				SetScroll(m_scroll + rect.m_mins.y);
			}
			else if (rect.m_maxs.y > m_pageSize.y)
			{
				SetScroll(m_scroll + rect.m_maxs.y - m_pageSize.y);
			}

			break;
		}
	}
}

void Tree::EditSelected()
{
	EnsureSelectedVisible();
	for (size_t row = 0; row < m_rows.size(); ++row)
	{
		Item const& item = m_items[m_rows[row]];
		if (item.m_ID == m_selected && item.m_editable)
		{
			m_editing = item.m_ID;
			m_labels[row]->SetVisible(false);
			m_edit->SetText(item.m_text);
			UpdateRowLayout();
			m_edit->SetVisible(true);
			m_edit->GrabFocus();
			m_edit->SelectAll();
			break;
		}
	}
}

bool Tree::GuiInput(InputEvent const& event)
{
	if (HandleScrollInput(event))
	{
		return true;
	}

	auto callback = m_inputCallback;
	if (callback && callback(event))
	{
		return true;
	}

	switch (event.m_type)
	{
	case InputEventType::MouseButton:
	{
		if (event.m_mouseButton != 0 || !event.m_pressed)
		{
			break;
		}

		int const row = GetRowAtPosition(event.m_position);
		if (row < 0)
		{
			Select(0, true);
			return true;
		}

		uint64_t const id = m_items[m_rows[row]].m_ID;
		for (int action = 0; action < static_cast<int>(m_items[m_rows[row]].m_actions.size()); ++action)
		{
			if (GetItemActionRect(id, action).IsPointInside(event.m_position))
			{
				if (m_items[m_rows[row]].m_actions[action].m_enabled && m_itemActionCallback)
				{
					m_itemActionCallback(id, action);
				}
				return true;
			}
		}

		Select(id, true);
		float const foldEnd =
			GetRowTextX(row)
			- (m_items[m_rows[row]].m_icon.IsValid() ? m_themeCache.icon_size + m_themeCache.icon_separation : 0);
		if (event.m_position.x < foldEnd && HasChildren(id))
		{
			SetCollapsed(id, !IsCollapsed(id));
		}
		else
		{
			double const now = glfwGetTime();
			if (m_lastClicked == id && now - m_lastClickTime < 0.35 && m_activatedCallback)
			{
				m_activatedCallback(id);
			}

			m_lastClicked   = id;
			m_lastClickTime = now;
		}

		return true;
	}
	case InputEventType::MouseMotion:
	{
		uint64_t const hovered = GetItemAtPosition(event.m_position);
		if (m_hoveredItem != hovered)
		{
			m_hoveredItem = hovered;
			QueueRedraw();
		}

		break;
	}
	case InputEventType::Key:
	{
		if (!event.m_pressed)
		{
			return false;
		}

		auto it = std::find_if(
			m_rows.begin(),
			m_rows.end(),
			[this](size_t index)
			{
				return m_items[index].m_ID == m_selected;
			});
		int row = it == m_rows.end() ? -1 : static_cast<int>(it - m_rows.begin());
		if ((event.m_keyCode == GLFW_KEY_UP || event.m_keyCode == GLFW_KEY_DOWN) && !m_rows.empty())
		{
			row = std::clamp(row + (event.m_keyCode == GLFW_KEY_UP ? -1 : 1), 0, static_cast<int>(m_rows.size()) - 1);
			Select(m_items[m_rows[row]].m_ID, true);
			return true;
		}

		if (row < 0)
		{
			return false;
		}

		Item const item = m_items[m_rows[row]];
		if (event.m_keyCode == GLFW_KEY_LEFT)
		{
			if (HasChildren(item.m_ID) && !IsCollapsed(item.m_ID))
			{
				SetCollapsed(item.m_ID, true);
			}
			else if (item.m_parentID != 0)
			{
				Select(item.m_parentID, true);
			}
		}
		else if (event.m_keyCode == GLFW_KEY_RIGHT)
		{
			if (HasChildren(item.m_ID))
			{
				if (IsCollapsed(item.m_ID))
				{
					SetCollapsed(item.m_ID, false);
				}
				else if (row + 1 < static_cast<int>(m_rows.size()))
				{
					Select(m_items[m_rows[row + 1]].m_ID, true);
				}
			}
		}
		else if (event.m_keyCode == GLFW_KEY_F2)
		{
			EditSelected();
		}
		else if (event.m_keyCode == GLFW_KEY_ENTER && m_activatedCallback)
		{
			m_activatedCallback(item.m_ID);
		}
		else
		{
			return false;
		}

		return true;
	}
	}

	return false;
}

uint64_t Tree::GetItemAtPosition(Vector2 const& position) const
{
	int const row = GetRowAtPosition(position);
	return row >= 0 ? m_items[m_rows[row]].m_ID : 0;
}

bool Tree::GetDragData(Vector2 const& position, GuiDragData& data)
{
	uint64_t const id = GetItemAtPosition(position);
	if (id == 0 || m_editing != 0)
	{
		return false;
	}

	int const row = GetRowAtPosition(position);
	if (position.x >= GetRowTextEnd(row))
	{
		return false;
	}

	float const foldEnd =
		GetRowTextX(row)
		- (m_items[m_rows[row]].m_icon.IsValid() ? m_themeCache.icon_size + m_themeCache.icon_separation : 0);
	if (HasChildren(id) && position.x < foldEnd)
	{
		return false;
	}

	return Control::GetDragData(position, data);
}

void Tree::DragHover(Vector2 const& position, GuiDragData const& data, bool accepted)
{
	uint64_t const item = accepted ? GetItemAtPosition(position) : 0;
	if (item != m_dropItem)
	{
		m_dropItem = item;
		QueueRedraw();
	}

	Control::DragHover(position, data, accepted);
}

void Tree::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Resized:
	case Notification_MinimumSizeChanged:
		UpdateRowLayout();
		break;
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		m_dragAxis = -1;
		[[fallthrough]];
	case Notification_MouseExit:
		m_hoverAxis = -1;
		m_hoveredItem = 0;
		QueueRedraw();
		break;
	case Notification_FocusEnter:
	case Notification_FocusExit:
		QueueRedraw();
		break;
	case Notification_Draw:
		DrawThemeStyleBox(m_themeCache.panel, GetLocalRect());
		for (size_t row = 0; row < m_rows.size(); ++row)
		{
			uint64_t const id = m_items[m_rows[row]].m_ID;
			if (id == m_selected)
			{
				DrawThemeStyleBox(m_themeCache.selected, ClipItemRect(GetRowRect(row)));
			}
			else if (id == m_hoveredItem)
			{
				DrawThemeStyleBox(m_themeCache.hover, ClipItemRect(GetRowRect(row)));
			}
		}

		if (m_themeCache.guide_width > 0)
		{
			int selectedRow = -1;
			for (int row = 0; row < static_cast<int>(m_rows.size()); ++row)
			{
				if (m_items[m_rows[row]].m_ID == m_selected)
				{
					selectedRow = row;
					break;
				}
			}

			if (selectedRow >= 0 && m_themeCache.children_guide_width > 0)
			{
				int         lastChild = -1;
				float const left      = m_themeCache.panel.IsValid()
											? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Left)
											: 0.f;
				float const x = left - m_horizontalScroll + m_depths[selectedRow] * m_themeCache.indent + GetFoldWidth(selectedRow) * 0.5f;
				float const width = static_cast<float>(m_themeCache.children_guide_width);
				for (int row = selectedRow + 1;
					 row < static_cast<int>(m_rows.size()) && m_depths[row] > m_depths[selectedRow];
					 ++row)
				{
					if (m_items[m_rows[row]].m_parentID != m_selected)
					{
						continue;
					}

					float const y   = GetRowRect(row).GetCenter().y;
					float const end = left - m_horizontalScroll + m_depths[row] * m_themeCache.indent
										+ (HasChildren(m_items[m_rows[row]].m_ID) ? 0.f : GetFoldWidth(row))
									  - m_themeCache.icon_separation;
					DrawItemRect(AABB2(x, y, std::max(x, end), y + width), m_themeCache.children_guide_color);
					lastChild = row;
				}

				if (lastChild >= 0)
				{
					DrawItemRect(
						AABB2(
							x,
							GetRowRect(selectedRow).GetCenter().y + GetFoldWidth(selectedRow) * 0.5f,
							x + width,
							GetRowRect(lastChild).GetCenter().y + width),
						m_themeCache.children_guide_color);
				}
			}

			for (int child = selectedRow; child > 0;)
			{
				int parent = child - 1;
				while (parent >= 0 && m_depths[parent] >= m_depths[child])
				{
					--parent;
				}

				if (parent < 0)
				{
					break;
				}

				float const left   = m_themeCache.panel.IsValid()
										 ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Left)
										 : 0.f;
				float const x      = left - m_horizontalScroll + m_depths[parent] * m_themeCache.indent + GetFoldWidth(parent) * 0.5f;
				float const top    = GetRowRect(parent).GetCenter().y + GetFoldWidth(parent) * 0.5f;
				float const bottom = GetRowRect(child).GetCenter().y;
				float const end    = left - m_horizontalScroll + m_depths[child] * m_themeCache.indent
									+ (HasChildren(m_items[m_rows[child]].m_ID) ? 0.f : GetFoldWidth(child))
								   - m_themeCache.icon_separation;
				DrawItemRect(AABB2(x, top, x + m_themeCache.guide_width, bottom), m_themeCache.guide_color);
				DrawItemRect(
					AABB2(x, bottom, std::max(x, end), bottom + m_themeCache.guide_width),
					m_themeCache.guide_color);
				child = parent;
			}
		}

		for (size_t row = 0; row < m_rows.size(); ++row)
		{
			uint64_t const id = m_items[m_rows[row]].m_ID;

			bool const texturedFold = m_themeCache.arrow_collapsed.IsValid() && m_themeCache.arrow_expanded.IsValid();
			if (texturedFold && HasChildren(id))
			{
				auto arrow =
					IsCollapsed(id) && !m_filtering ? m_themeCache.arrow_collapsed : m_themeCache.arrow_expanded;
				float const iconWidth =
					m_items[m_rows[row]].m_icon.IsValid() ? m_themeCache.icon_size + m_themeCache.icon_separation : 0.f;
				Rect2       target;
				float const foldSize = GetFoldWidth(row);
				target.m_position =
					Vector2(GetRowTextX(row) - foldSize - iconWidth, GetRowRect(row).GetCenter().y - foldSize * 0.5f);
				target.m_size = Vector2(foldSize, foldSize);
				Rect2 source;
				source.m_size = Vector2(arrow->GetDimensions());
				DrawItemTexture(arrow, target, source, m_themeCache.guide_color);
			}

			if (auto icon = m_items[m_rows[row]].m_icon; icon.IsValid())
			{
				AABB2 const rowRect = GetRowRect(row);
				float const size    = static_cast<float>(std::max(1, m_themeCache.icon_size));
				Rect2       target;
				target.m_position = Vector2(
					GetRowTextX(row) - (texturedFold ? 0.f : GetFoldWidth(row)) - size - m_themeCache.icon_separation,
					rowRect.m_mins.y + (rowRect.GetHeight() - size) * 0.5f);
				target.m_size = Vector2(size, size);
				Rect2 source;
				source.m_size = Vector2(icon->GetDimensions());
				DrawItemTexture(icon, target, source, m_items[m_rows[row]].m_iconColor);
			}

			for (int action = 0; action < static_cast<int>(m_items[m_rows[row]].m_actions.size()); ++action)
			{
				auto const& entry = m_items[m_rows[row]].m_actions[action];
				if (entry.m_icon.IsValid())
				{
					AABB2 const bounds = GetItemActionRect(id, action);
					Rect2       target;
					target.m_position = bounds.m_mins;
					target.m_size     = bounds.GetDimensions();
					Rect2 source;
					source.m_size = Vector2(entry.m_icon->GetDimensions());
					DrawItemTexture(entry.m_icon, target, source, entry.m_color);
				}
			}

			if (id == m_dropItem)
			{
				DrawThemeStyleBox(m_themeCache.drop_mark, ClipItemRect(GetRowRect(row)));
			}
		}

		for (int axis = 0; axis < 2; ++axis)
		{
			AABB2 const track = GetTrackRect(axis);
			if (track.GetWidth() <= 0.f || track.GetHeight() <= 0.f)
			{
				continue;
			}

			DrawThemeStyleBox(m_themeCache.track, track);
			Ref<ThemeStyleBox> const style =
				m_dragAxis == axis ? m_themeCache.grabber_pressed
								   : (m_hoverAxis == axis ? m_themeCache.grabber_hover : m_themeCache.grabber);
			DrawThemeStyleBox(style, GetGrabberRect(axis));
		}

		if (HasFocus())
		{
			DrawThemeStyleBox(m_themeCache.focus, GetLocalRect());
		}
		break;
	}
}

AABB2 Tree::GetTrackRect(int axis) const
{
	float const length = axis == 0 ? m_pageSize.x : m_pageSize.y;
	float const extent = axis == 0 ? m_contentSize.x : m_contentSize.y;
	if (extent <= length || length <= 0.f || m_themeCache.scrollbar_width <= 0)
	{
		return AABB2::Zero;
	}

	return axis == 0 ? AABB2(0.f, m_pageSize.y, length, GetSize().y) : AABB2(m_pageSize.x, 0.f, GetSize().x, length);
}

AABB2 Tree::GetGrabberRect(int axis) const
{
	AABB2 const track  = GetTrackRect(axis);
	float const length = axis == 0 ? track.GetWidth() : track.GetHeight();
	if (track.GetWidth() <= 0.f || track.GetHeight() <= 0.f)
	{
		return AABB2::Zero;
	}

	float const extent = axis == 0 ? m_contentSize.x : m_contentSize.y;
	float const offset = axis == 0 ? m_horizontalScroll : m_scroll;
	float const thumb  = std::min(
		length,
		std::max(static_cast<float>(std::max(1, m_themeCache.grabber_minimum_size)), length * length / extent));
	float const top       = offset / (extent - length) * (length - thumb);
	float const thickness = axis == 0 ? track.GetHeight() : track.GetWidth();
	float const margin =
		std::min(static_cast<float>(std::max(0, m_themeCache.grabber_margin)), std::max(0.f, (thickness - 1.f) * 0.5f));
	return axis == 0 ? AABB2(top, track.m_mins.y + margin, top + thumb, track.m_maxs.y - margin)
					 : AABB2(track.m_mins.x + margin, top, track.m_maxs.x - margin, top + thumb);
}

bool Tree::HandleScrollInput(InputEvent const& event)
{
	if (event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseButton)
	{
		int hovered = -1;
		for (int axis = 0; axis < 2; ++axis)
		{
			AABB2 const thumb = GetGrabberRect(axis);
			if (thumb.GetWidth() > 0.f && thumb.GetHeight() > 0.f && thumb.IsPointInside(event.m_position))
			{
				hovered = axis;
			}
		}

		if (hovered != m_hoverAxis)
		{
			m_hoverAxis = hovered;
			QueueRedraw();
		}
	}

	switch (event.m_type)
	{
	case InputEventType::MouseWheel:
	{
		float const step = static_cast<float>(std::max(0, m_themeCache.scroll_step));
		if ((event.m_modifiers & GLFW_MOD_SHIFT) != 0)
		{
			SetHorizontalScroll(m_horizontalScroll - event.m_scroll.y * step);
		}
		else
		{
			m_horizontalScroll -= event.m_scroll.x * step;
			SetScroll(m_scroll - event.m_scroll.y * step);
		}

		return true;
	}
	case InputEventType::MouseButton:
	{
		if (event.m_mouseButton != GLFW_MOUSE_BUTTON_LEFT)
		{
			return IsPointInside(event.m_position) && !AABB2(Vector2::Zero, m_pageSize).IsPointInside(event.m_position);
		}

		if (!event.m_pressed)
		{
			bool const dragging = m_dragAxis >= 0;
			m_dragAxis = -1;
			QueueRedraw();
			return dragging;
		}

		m_dragAxis = -1;
		for (int axis = 0; axis < 2; ++axis)
		{
			AABB2 const track = GetTrackRect(axis);
			if (track.GetWidth() > 0.f && track.GetHeight() > 0.f && track.IsPointInside(event.m_position))
			{
				m_dragAxis = axis;
				break;
			}
		}

		if (m_dragAxis < 0)
		{
			return IsPointInside(event.m_position) && !AABB2(Vector2::Zero, m_pageSize).IsPointInside(event.m_position);
		}

		AABB2 const thumb    = GetGrabberRect(m_dragAxis);
		float const top      = m_dragAxis == 0 ? thumb.m_mins.x : thumb.m_mins.y;
		float const size     = m_dragAxis == 0 ? thumb.GetWidth() : thumb.GetHeight();
		float const position = m_dragAxis == 0 ? event.m_position.x : event.m_position.y;
		m_dragOffset         = position >= top && position < top + size ? position - top : size * 0.5f;
		m_dragScrollbarWidth = m_themeCache.scrollbar_width;
		m_dragMinimumSize    = m_themeCache.grabber_minimum_size;
		m_dragMargin         = m_themeCache.grabber_margin;
		QueueRedraw();
	}
		[[fallthrough]];
	case InputEventType::MouseMotion:
	{
		if (m_dragAxis < 0)
		{
			break;
		}

		AABB2 const track    = GetTrackRect(m_dragAxis);
		AABB2 const thumb    = GetGrabberRect(m_dragAxis);
		float const length   = m_dragAxis == 0 ? track.GetWidth() : track.GetHeight();
		float const size     = m_dragAxis == 0 ? thumb.GetWidth() : thumb.GetHeight();
		float const extent   = m_dragAxis == 0 ? m_contentSize.x : m_contentSize.y;
		float const position = m_dragAxis == 0 ? event.m_position.x : event.m_position.y;
		float const offset   = length > size ? (position - m_dragOffset) / (length - size) * (extent - length) : 0.f;
		if (m_dragAxis == 0)
		{
			SetHorizontalScroll(offset);
		}
		else
		{
			SetScroll(offset);
		}

		return true;
	}
	}

	return false;
}

float Tree::GetMaximumScroll() const
{
	return std::max(0.f, m_contentSize.y - m_pageSize.y);
}

void Tree::UpdateScrollRange()
{
	Vector2 const previousPage = m_pageSize;
	Vector2 const previousContent = m_contentSize;
	float const thickness = static_cast<float>(std::max(0, m_themeCache.scrollbar_width));
	Vector2 const margins = m_themeCache.panel.IsValid() ? m_themeCache.panel->GetMinimumSize() : Vector2::Zero;
	float const height = margins.y + m_rows.size() * GetRowHeight();
	bool horizontal = false;
	bool vertical = false;
	for (int i = 0; i < 3; ++i)
	{
		m_pageSize = Vector2(std::max(0.f, GetSize().x - (vertical ? thickness : 0.f)),
			std::max(0.f, GetSize().y - (horizontal ? thickness : 0.f)));
		horizontal = !m_clipText && m_contentWidth > m_pageSize.x;
		vertical = height > m_pageSize.y;
	}

	m_pageSize = Vector2(std::max(0.f, GetSize().x - (vertical ? thickness : 0.f)),
		std::max(0.f, GetSize().y - (horizontal ? thickness : 0.f)));
	m_contentSize = Vector2(m_clipText ? m_pageSize.x : std::max(m_contentWidth, m_pageSize.x),
		std::max(height, m_pageSize.y));
	if (previousPage != m_pageSize || previousContent != m_contentSize)
	{
		m_dragAxis = -1;
		m_hoverAxis = -1;
	}

	m_scroll = std::clamp(m_scroll, 0.f, GetMaximumScroll());
	m_horizontalScroll = std::clamp(m_horizontalScroll, 0.f, std::max(0.f, m_contentSize.x - m_pageSize.x));
	SetChildrenClip(true, m_pageSize);
}

void Tree::SetScroll(float offset)
{
	if (!std::isfinite(offset))
	{
		return;
	}

	m_scroll = offset;
	m_hoveredItem = 0;
	UpdateRowLayout();
}

void Tree::SetHorizontalScroll(float offset)
{
	if (!std::isfinite(offset))
	{
		return;
	}

	m_horizontalScroll = offset;
	m_hoveredItem = 0;
	UpdateRowLayout();
}

AABB2 Tree::ClipItemRect(AABB2 const& rect) const
{
	return AABB2(Vector2(std::clamp(rect.m_mins.x, 0.f, m_pageSize.x), std::clamp(rect.m_mins.y, 0.f, m_pageSize.y)),
		Vector2(std::clamp(rect.m_maxs.x, 0.f, m_pageSize.x), std::clamp(rect.m_maxs.y, 0.f, m_pageSize.y)));
}

void Tree::DrawItemRect(AABB2 const& rect, Color const& color)
{
	AABB2 const clipped = ClipItemRect(rect);
	if (clipped.GetWidth() > 0.f && clipped.GetHeight() > 0.f)
	{
		DrawRect(clipped, color);
	}
}

void Tree::DrawItemTexture(
	Ref<TextureResource> const& texture, Rect2 const& rect, Rect2 const& source, Color const& color)
{
	if (rect.m_size.x <= 0.f || rect.m_size.y <= 0.f)
	{
		return;
	}

	AABB2 const clipped = ClipItemRect(AABB2(rect.m_position, rect.m_position + rect.m_size));
	if (clipped.GetWidth() <= 0.f || clipped.GetHeight() <= 0.f)
	{
		return;
	}

	Rect2 target;
	target.m_position = clipped.m_mins;
	target.m_size = clipped.GetDimensions();
	Rect2 region;
	region.m_position = source.m_position + Vector2(
		(clipped.m_mins.x - rect.m_position.x) / rect.m_size.x * source.m_size.x,
		(clipped.m_mins.y - rect.m_position.y) / rect.m_size.y * source.m_size.y);
	region.m_size = Vector2(target.m_size.x / rect.m_size.x * source.m_size.x,
		target.m_size.y / rect.m_size.y * source.m_size.y);
	DrawTextureRectRegion(texture, target, region, color);
}

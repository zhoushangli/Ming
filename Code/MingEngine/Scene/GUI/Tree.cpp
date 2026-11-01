#include "MingEngine/Scene/GUI/Tree.hpp"

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"

#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <unordered_map>

Tree::Tree()
{
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
	BIND_THEME_ITEM(Theme::DataType::Icon, Tree, arrow_collapsed);
	BIND_THEME_ITEM(Theme::DataType::Icon, Tree, arrow_expanded);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, panel);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, hover);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, selected);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, focus);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Tree, drop_mark);
}

void Tree::SetFont(Ref<FontResource> const& font)
{
	AddThemeFontOverride("font", font);
}

void Tree::OnThemeChanged()
{
	UpdateRowLayout();
	EnsureSelectedVisible();
}

float Tree::GetRowHeight() const
{
	float height = static_cast<float>(std::max(1, m_themeCache.font_size));
	if (m_themeCache.font.IsValid() && m_themeCache.font->GetFontRID().IsValid()
		&& g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		auto const line = g_engine->m_textServer->GetTextLine(
			m_themeCache.font->GetFontRID(), "W", static_cast<float>(m_themeCache.font_size));
		height = std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent);
	}

	return std::max({ 1.f, height, static_cast<float>(m_themeCache.row_height),
		m_edit != nullptr ? m_edit->GetCombinedMinimumSize().y : 0.f });
}

AABB2 Tree::GetRowRect(size_t row) const
{
	float const top = (m_themeCache.panel.IsValid()
		? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Top) : 0.f) + row * GetRowHeight();
	return AABB2(0.f, top, GetSize().x, top + GetRowHeight());
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
	float const top = m_themeCache.panel.IsValid()
		? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Top) : 0.f;
	if (!IsPointInside(position) || position.y < top)
	{
		return -1;
	}

	int const row = static_cast<int>((position.y - top) / GetRowHeight());
	return row < static_cast<int>(m_rows.size()) ? row : -1;
}

float Tree::GetFoldWidth(size_t row) const
{
	if (m_themeCache.arrow_collapsed.IsValid() && m_themeCache.arrow_expanded.IsValid())
	{
		return static_cast<float>(std::max(0, m_themeCache.fold_size));
	}

	String const prefix(HasChildren(m_items[m_rows[row]].m_ID)
		? (IsCollapsed(m_items[m_rows[row]].m_ID) && !m_filtering ? "> " : "v ") : "  ");
	if (m_themeCache.font.IsValid() && m_themeCache.font->GetFontRID().IsValid()
		&& g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		return g_engine->m_textServer->GetTextLine(
			m_themeCache.font->GetFontRID(), prefix, static_cast<float>(m_themeCache.font_size)).m_width;
	}

	return static_cast<float>(std::max(1, m_themeCache.font_size));
}

float Tree::GetRowTextX(size_t row) const
{
	float const left = m_themeCache.panel.IsValid()
		? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Left) : 0.f;
	float const iconWidth = m_items[m_rows[row]].m_icon.IsValid()
		? static_cast<float>(m_themeCache.icon_size + m_themeCache.icon_separation) : 0.f;
	return left + m_depths[row] * static_cast<float>(std::max(0, m_themeCache.indent)) + GetFoldWidth(row) + iconWidth;
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
	float const left = m_themeCache.panel.IsValid()
		? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Left) : 0.f;
	float const right = m_themeCache.panel.IsValid()
		? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Right) : 0.f;
	float width = std::max(1.f, left + right);
	for (size_t row = 0; row < m_rows.size(); ++row)
	{
		Item const& item = m_items[m_rows[row]];
		Label* label = m_labels[row];
		bool const texturedFold = m_themeCache.arrow_collapsed.IsValid() && m_themeCache.arrow_expanded.IsValid();
		label->SetText(texturedFold ? item.m_text : String(HasChildren(item.m_ID)
			? (IsCollapsed(item.m_ID) && !m_filtering ? "> " : "v ") : "  ") + item.m_text);
		label->SetFont(m_themeCache.font);
		label->SetFontSize(m_themeCache.font_size);
		label->SetFontColor(item.m_ID == m_selected ? m_themeCache.font_selected_color : m_themeCache.font_color);
		AABB2 const rect = GetRowRect(row);
		label->SetPosition(Vector2(GetRowTextX(row) - (texturedFold ? 0.f : GetFoldWidth(row)), rect.m_mins.y));
		label->UpdateSize(Vector2(label->GetCombinedMinimumSize().x, rect.GetHeight()));
		width = std::max(width, label->GetLocalPosition().x + label->GetSize().x + right);
		if (item.m_ID == m_editing)
		{
			m_edit->UpdateSize(Vector2(std::max(m_edit->GetCombinedMinimumSize().x, GetSize().x - GetRowTextX(row)), rect.GetHeight()));
			float const x = GetRowTextX(row) - m_edit->GetContentRect().m_mins.x;
			m_edit->SetPosition(Vector2(x, rect.m_mins.y));
			m_edit->UpdateSize(Vector2(std::max(m_edit->GetCombinedMinimumSize().x, GetSize().x - x - right), rect.GetHeight()));
		}
	}

	bool const widthChanged = m_contentWidth != width;
	m_contentWidth = width;
	m_updatingRowLayout = false;
	if (widthChanged)
	{
		PropagateMinimumSizeChanged();
	}

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
			   && items[i].m_iconColor == m_items[i].m_iconColor;
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
	float const margins = m_themeCache.panel.IsValid() ? m_themeCache.panel->GetMinimumSize().y : 0.f;
	return Vector2(m_contentWidth, margins + std::max(1.f, static_cast<float>(m_rows.size())) * GetRowHeight());
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
			if (auto* scroll = dynamic_cast<ScrollContainer*>(GetParent()))
			{
				scroll->EnsureControlVisible(m_labels[row]);
			}

			break;
		}
	}
}

void Tree::EditSelected()
{
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
		Select(id, true);
		float const foldEnd = GetRowTextX(row) - (m_items[m_rows[row]].m_icon.IsValid()
			? m_themeCache.icon_size + m_themeCache.icon_separation : 0);
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
	float const foldEnd = GetRowTextX(row) - (m_items[m_rows[row]].m_icon.IsValid()
		? m_themeCache.icon_size + m_themeCache.icon_separation : 0);
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
	case Notification_MouseExit:
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
				DrawThemeStyleBox(m_themeCache.selected, GetRowRect(row));
			}
			else if (id == m_hoveredItem)
			{
				DrawThemeStyleBox(m_themeCache.hover, GetRowRect(row));
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

				float const left = m_themeCache.panel.IsValid() ? m_themeCache.panel->GetContentMargin(ThemeStyleBox::Side::Left) : 0.f;
				float const x = left + m_depths[parent] * m_themeCache.indent + GetFoldWidth(parent) * 0.5f;
				float const top = GetRowRect(parent).GetCenter().y + 7.f;
				float const bottom = GetRowRect(child).GetCenter().y;
				float const end = left + m_depths[child] * m_themeCache.indent - 5.f;
				DrawRect(AABB2(x, top, x + m_themeCache.guide_width, bottom), m_themeCache.guide_color);
				DrawRect(AABB2(x, bottom, std::max(x, end), bottom + m_themeCache.guide_width), m_themeCache.guide_color);
				child = parent;
			}
		}

		for (size_t row = 0; row < m_rows.size(); ++row)
		{
			uint64_t const id = m_items[m_rows[row]].m_ID;

			bool const texturedFold = m_themeCache.arrow_collapsed.IsValid() && m_themeCache.arrow_expanded.IsValid();
			if (texturedFold && HasChildren(id))
			{
				auto arrow = IsCollapsed(id) && !m_filtering ? m_themeCache.arrow_collapsed : m_themeCache.arrow_expanded;
				float const iconWidth = m_items[m_rows[row]].m_icon.IsValid() ? m_themeCache.icon_size + m_themeCache.icon_separation : 0.f;
				Rect2 target;
				float const foldSize = GetFoldWidth(row);
				target.m_position = Vector2(GetRowTextX(row) - foldSize - iconWidth, GetRowRect(row).GetCenter().y - foldSize * 0.5f);
				target.m_size = Vector2(foldSize, foldSize);
				Rect2 source;
				source.m_size = Vector2(arrow->GetDimensions());
				DrawTextureRectRegion(arrow, target, source, m_themeCache.font_color);
			}

			if (auto icon = m_items[m_rows[row]].m_icon; icon.IsValid())
			{
				AABB2 const rowRect = GetRowRect(row);
				float const size = static_cast<float>(std::max(1, m_themeCache.icon_size));
				Rect2 target;
				target.m_position = Vector2(GetRowTextX(row) - (texturedFold ? 0.f : GetFoldWidth(row))
					- size - m_themeCache.icon_separation, rowRect.m_mins.y + (rowRect.GetHeight() - size) * 0.5f);
				target.m_size = Vector2(size, size);
				Rect2 source;
				source.m_size = Vector2(icon->GetDimensions());
				DrawTextureRectRegion(icon, target, source, m_items[m_rows[row]].m_iconColor);
			}

			if (id == m_dropItem)
			{
				DrawThemeStyleBox(m_themeCache.drop_mark, GetRowRect(row));
			}
		}

		if (HasFocus())
		{
			DrawThemeStyleBox(m_themeCache.focus, GetLocalRect());
		}
		break;
	}
}

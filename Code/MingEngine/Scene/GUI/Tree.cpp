#include "MingEngine/Scene/GUI/Tree.hpp"

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

void Tree::SetFont(Ref<FontResource> const& font)
{
	if (m_font == font)
	{
		return;
	}

	m_font = font;
	m_edit->SetFont(font);
	RebuildRows();
}

void Tree::SetItems(std::vector<Item> items, bool filtering)
{
	bool same = m_filtering == filtering && items.size() == m_items.size();
	for (size_t i = 0; same && i < items.size(); ++i)
	{
		same = items[i].m_ID == m_items[i].m_ID && items[i].m_parentID == m_items[i].m_parentID
			   && items[i].m_text == m_items[i].m_text && items[i].m_visible == m_items[i].m_visible
			   && items[i].m_editable == m_items[i].m_editable;
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
		label->SetFont(m_font);
		label->SetFontSize(16);
		label->SetText(
			String(HasChildren(item.m_ID) ? (IsCollapsed(item.m_ID) && !m_filtering ? "> " : "v ") : "  ")
			+ item.m_text);
		label->SetPosition(Vector2(depth * 18.f + 4.f, static_cast<float>(m_rows.size()) * 28.f + 4.f));
		label->UpdateSize(label->GetCombinedMinimumSize());
		AddNode(label);
		m_contentWidth = std::max(m_contentWidth, label->GetLocalPosition().x + label->GetSize().x + 12.f);
		m_labels.push_back(label);
		m_rows.push_back(i);
		m_depths.push_back(depth);
	}

	PropagateMinimumSizeChanged();
	QueueRedraw();
}

Vector2 Tree::GetInternalMinimumSize() const
{
	return Vector2(m_contentWidth, std::max(28.f, static_cast<float>(m_rows.size()) * 28.f));
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
			m_edit->SetPosition(Vector2(m_depths[row] * 18.f + 24.f, row * 28.f));
			m_edit->UpdateSize(Vector2(std::max(160.f, GetSize().x - m_edit->GetLocalPosition().x), 28.f));
			m_edit->SetVisible(true);
			m_edit->GrabFocus();
			m_edit->SelectAll();
			break;
		}
	}
}

bool Tree::GuiInput(InputEvent const& event)
{
	switch (event.m_type)
	{
	case InputEventType::MouseButton:
	{
		if (event.m_mouseButton != 0 || !event.m_pressed)
		{
			break;
		}

		int const row = static_cast<int>(event.m_position.y / 28.f);
		if (event.m_position.y < 0.f || row >= static_cast<int>(m_rows.size()))
		{
			Select(0, true);
			return true;
		}

		uint64_t const id = m_items[m_rows[row]].m_ID;
		Select(id, true);
		if (event.m_position.x < m_depths[row] * 18.f + 24.f && HasChildren(id))
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
	if (!IsPointInside(position))
	{
		return 0;
	}

	size_t const row = static_cast<size_t>(position.y / 28.f);
	return row < m_rows.size() ? m_items[m_rows[row]].m_ID : 0;
}

bool Tree::GetDragData(Vector2 const& position, GuiDragData& data)
{
	uint64_t const id = GetItemAtPosition(position);
	if (id == 0 || m_editing != 0)
	{
		return false;
	}

	size_t const row = static_cast<size_t>(position.y / 28.f);
	if (HasChildren(id) && position.x < m_depths[row] * 18.f + 24.f)
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
	case Notification_Draw:
		for (size_t row = 0; row < m_rows.size(); ++row)
		{
			if (m_items[m_rows[row]].m_ID == m_selected)
			{
				DrawRect(AABB2(Vector2(0.f, row * 28.f), Vector2(GetSize().x, (row + 1) * 28.f)), Color(52, 82, 119));
			}

			if (m_items[m_rows[row]].m_ID == m_dropItem)
			{
				DrawRect(AABB2(Vector2(0.f, row * 28.f), Vector2(GetSize().x, row * 28.f + 2.f)), Color(92, 180, 240));
				DrawRect(AABB2(Vector2(0.f, (row + 1) * 28.f - 2.f), Vector2(GetSize().x, (row + 1) * 28.f)), Color(92, 180, 240));
			}
		}
		break;
	}
}

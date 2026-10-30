#include "MingEngine/Scene/GUI/TabContainer.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"

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
	int index = position.y < 40.f ? GetTabInsertIndex(position) : GetPageCount();
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
	SetDropInsertIndex(accepted && position.y >= 0.f && position.y < 40.f ? GetTabInsertIndex(position) : -1);
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
		float x = 0.f;
		for (Button* button : m_buttons)
		{
			float const width = std::max(72.f, button->GetCombinedMinimumSize().x);
			button->SetPosition(Vector2(x, 0.f));
			button->UpdateSize(Vector2(width, 36.f));
			x += width + 4.f;
		}

		if (Control* page = GetCurrentPage())
		{
			page->SetPosition(Vector2(0.f, 40.f));
			page->UpdateSize(Vector2(GetSize().x, std::max(0.f, GetSize().y - 40.f)));
		}
		break;
	}
	case Notification_Draw:
		if (m_dropIndex >= 0)
		{
			float const x = m_dropIndex < GetPageCount() ? m_buttons[m_dropIndex]->GetLocalPosition().x
				: (m_buttons.empty() ? 0.f : m_buttons.back()->GetLocalPosition().x + m_buttons.back()->GetSize().x);
			DrawRect(AABB2(Vector2(x, 0.f), Vector2(x + 3.f, 36.f)), Color(110, 195, 245));
		}
		break;
	}
}

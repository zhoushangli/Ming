#include "MingEngine/Scene/GUI/TabContainer.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"

#include <algorithm>

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
	int const index  = static_cast<int>(m_pages.size());
	auto*     button = MemNew<Button>();
	button->SetText(title);
	button->SetToggleMode(true);
	button->SetPressedCallback(
		[this, index]()
		{
			SetCurrentTab(index);
		});
	AddNode(button);
	AddNode(page);
	m_buttons.push_back(button);
	m_pages.push_back(page);
	page->SetVisible(false);
	if (m_current < 0)
	{
		SetCurrentTab(index);
	}

	MarkDirty();
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

			viewport->CancelGuiInteraction(previous);
		}

		previous->SetVisible(false);
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
	if (notification == Notification_UpdateLayout)
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
	}
}

#include "MingEngine/Scene/GUI/PopupPanel.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"

PopupPanel::PopupPanel()
{
	SetFocusMode(FocusMode::All);
	SetVisible(false);
}

void PopupPanel::Open(Control* owner)
{
	if (GetViewport() == nullptr || owner == nullptr)
	{
		return;
	}

	m_parentPopup = ObjectID::Invalid;
	if (auto* previous = dynamic_cast<PopupPanel*>(GetViewport()->GetGuiPopup()))
	{
		if (previous->IsAncestorOf(owner))
		{
			m_parentPopup = previous->GetObjectID();
		}
		else
		{
			previous->Close(false);
		}
	}

	if (!SetThemeSource(owner))
	{
		return;
	}

	m_interrupted   = false;
	m_owner         = owner->GetObjectID();
	Control* focus  = GetViewport()->GetGuiFocus();
	m_previousFocus = focus != nullptr ? focus->GetObjectID() : m_owner;
	if (focus != nullptr && focus->GetObjectID() == m_parentPopup)
	{
		m_previousFocus = m_owner;
		GetViewport()->SetGuiFocus(nullptr);
	}

	GetViewport()->CancelGuiInteraction(GetViewport());
	UpdateSize(Vector2(GetViewport()->GetOutputResolution()));
	SetVisible(true);
	GetViewport()->SetGuiPopup(this);
	GrabFocus();
}

void PopupPanel::Close(bool restoreFocus)
{
	if (GetViewport() != nullptr)
	{
		auto* child = dynamic_cast<PopupPanel*>(GetViewport()->GetGuiPopup());
		if (child != nullptr && child != this && child->m_parentPopup == GetObjectID())
		{
			child->Close(false);
		}
	}

	bool const wasVisible = IsVisible();
	if (GetViewport() != nullptr && GetViewport()->GetGuiPopup() == this)
	{
		auto* parent = ObjectDatabase::GetInstance<PopupPanel>(m_parentPopup);
		GetViewport()->SetGuiPopup(parent != nullptr && parent->IsVisibleInHierarchy() ? parent : nullptr);
	}

	SetVisible(false);
	Control* owner = ObjectDatabase::GetInstance<Control>(m_previousFocus);
	if (owner == nullptr || owner->IsQueuedForDeletion() || !owner->IsVisibleInHierarchy())
	{
		owner = ObjectDatabase::GetInstance<Control>(m_owner);
	}
	if (restoreFocus && owner != nullptr && !owner->IsQueuedForDeletion() && owner->IsVisibleInHierarchy())
	{
		owner->GrabFocus();
	}

	auto callback = m_closedCallback;
	if (wasVisible && callback)
	{
		callback();
	}
}

bool PopupPanel::GuiInput(InputEvent const& event)
{
	switch (event.m_type)
	{
	case InputEventType::MouseMotion:
	case InputEventType::MouseButton:
	{
		auto callback = m_pointerCallback;
		if (callback && callback(event))
		{
			return true;
		}
		if (event.m_type == InputEventType::MouseButton && !m_modal && event.m_pressed)
		{
			Close();
		}
		break;
	}
	case InputEventType::Key:
	case InputEventType::Text:
	{
		auto callback = m_keyCallback;
		if (callback)
		{
			callback(event);
		}

		break;
	}
	}

	return true;
}

void PopupPanel::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
		if (m_modal)
		{
			DrawRect(AABB2(Vector2::Zero, GetSize()), GetThemeColor("modal_color", "PopupPanel"));
		}
		break;

	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		if (IsVisible())
		{
			m_interrupted = true;
			Close(false);
		}
		break;
	}
}

void PopupPanel::OnThemeSourceLost()
{
	m_interrupted = true;
	Close(false);
	m_owner = ObjectID::Invalid;
}

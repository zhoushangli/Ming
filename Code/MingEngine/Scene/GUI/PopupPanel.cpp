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

	if (auto* previous = dynamic_cast<PopupPanel*>(GetViewport()->GetGuiPopup()))
	{
		previous->Close(false);
	}

	m_owner = owner->GetObjectID();
	GetViewport()->CancelGuiInteraction(GetViewport());
	UpdateSize(Vector2(GetViewport()->GetOutputResolution()));
	SetVisible(true);
	GetViewport()->SetGuiPopup(this);
	GrabFocus();
}

void PopupPanel::Close(bool restoreFocus)
{
	if (GetViewport() != nullptr && GetViewport()->GetGuiPopup() == this)
	{
		GetViewport()->SetGuiPopup(nullptr);
	}

	SetVisible(false);
	Control* owner = ObjectDatabase::GetInstance<Control>(m_owner);
	if (restoreFocus && owner != nullptr && !owner->IsQueuedForDeletion() && owner->IsVisibleInHierarchy())
	{
		owner->GrabFocus();
	}
}

bool PopupPanel::GuiInput(InputEvent const& event)
{
	if (event.m_type == InputEventType::Key || event.m_type == InputEventType::Text)
	{
		auto callback = m_keyCallback;
		if (callback)
		{
			callback(event);
		}

		return true;
	}

	if (!m_modal && event.m_type == InputEventType::MouseButton && event.m_pressed)
	{
		Close();
	}

	return true;
}

void PopupPanel::OnNotification(int notification)
{
	if (notification == Notification_GuiInputCancelled || notification == Notification_ExitTree)
	{
		if (GetViewport() != nullptr && GetViewport()->GetGuiPopup() == this)
		{
			GetViewport()->SetGuiPopup(nullptr);
		}

		if (IsVisible())
		{
			SetVisible(false);
		}
	}
}

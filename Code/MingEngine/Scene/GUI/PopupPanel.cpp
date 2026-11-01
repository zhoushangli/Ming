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

	if (!SetThemeSource(owner))
	{
		return;
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
	bool const wasVisible = IsVisible();
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

	auto callback = m_closedCallback;
	if (wasVisible && callback)
	{
		callback();
	}
}

bool PopupPanel::GuiInput(InputEvent const& event)
{
	if (event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseButton)
	{
		auto callback = m_pointerCallback;
		if (callback && callback(event))
		{
			return true;
		}
	}

	switch (event.m_type)
	{
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
	case InputEventType::MouseButton:
		if (!m_modal && event.m_pressed)
		{
			Close();
		}
		break;
	}

	return true;
}

void PopupPanel::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		Close(false);
		break;
	}
}

void PopupPanel::OnThemeSourceLost()
{
	Close(false);
	m_owner = ObjectID::Invalid;
}

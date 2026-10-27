#include "MingEngine/Scene/GUI/BaseButton.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"

BaseButton::BaseButton()
{
	SetFocusMode(FocusMode::All);
}

bool BaseButton::GuiInput(InputEvent const& event)
{
	if (m_status.m_disabled)
	{
		return false;
	}

	if (event.m_type == InputEventType::MouseMotion)
	{
		// If the button is being pressed, it needs update when cursor moving
		if (m_status.m_pressAttempt && m_status.m_mousePress)
		{
			bool const inside = IsPointInside(event.m_position);
			if (inside != m_status.m_pressingInside)
			{
				m_status.m_pressingInside = inside;
				QueueRedraw();
			}
		}

		return false;
	}

	bool const mouse    = event.m_type == InputEventType::MouseButton && event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT;
	bool const keyboard = event.m_type == InputEventType::Key && HasFocus()
						  && (event.m_keyCode == GLFW_KEY_ENTER || event.m_keyCode == GLFW_KEY_KP_ENTER
							  || event.m_keyCode == GLFW_KEY_SPACE);
	if ((!mouse && !keyboard) || event.m_repeat)
	{
		return false;
	}

	// Just button down
	if (event.m_pressed)
	{
		if (!m_status.m_pressAttempt && (!mouse || IsPointInside(event.m_position)))
		{
			m_status.m_pressAttempt   = true;
			m_status.m_pressingInside = true;
			m_status.m_mousePress     = mouse;
			m_status.m_activationKey  = keyboard ? event.m_keyCode : -1;
			QueueRedraw();
			auto callback = m_buttonDownCallback;
			if (callback)
			{
				callback();
			}
		}
	}
	// Pressed
	else if (
		m_status.m_pressAttempt && m_status.m_mousePress == mouse
		&& (mouse || m_status.m_activationKey == event.m_keyCode))
	{
		CompletePress(!mouse || IsPointInside(event.m_position));
	}

	return true;
}

void BaseButton::CompletePress(bool activate)
{
	if (!m_status.m_pressAttempt)
	{
		return;
	}

	// 1) Commit state before invoking callbacks that may change the scene
	activate                  = activate && !m_status.m_disabled && IsVisibleInHierarchy() && !IsQueuedForDeletion();
	bool const toggled        = activate && m_toggleMode;
	m_status.m_pressAttempt   = false;
	m_status.m_pressingInside = false;
	m_status.m_mousePress     = false;
	m_status.m_activationKey  = -1;
	if (toggled)
	{
		m_status.m_pressed = !m_status.m_pressed;
	}

	bool const     pressed = m_status.m_pressed;
	ObjectID const id      = GetObjectID();
	QueueRedraw();
	if (toggled)
	{
		auto callback = m_toggledCallback;
		if (callback)
		{
			callback(pressed);
		}
	}

	// 2) Resolve the button again between notifications
	BaseButton* button = ObjectDatabase::GetInstance<BaseButton>(id);
	if (button != nullptr && !button->IsQueuedForDeletion() && activate && !button->IsDisabled()
		&& button->IsVisibleInHierarchy() && button->GetSceneTree() != nullptr)
	{
		auto callback = button->m_pressedCallback;
		if (callback)
		{
			callback();
		}
	}

	button = ObjectDatabase::GetInstance<BaseButton>(id);
	if (button != nullptr && !button->IsQueuedForDeletion())
	{
		auto callback = button->m_buttonUpCallback;
		if (callback)
		{
			callback();
		}
	}
}

void BaseButton::SetDisabled(bool disabled)
{
	if (m_status.m_disabled == disabled)
	{
		return;
	}

	m_status.m_disabled = disabled;
	QueueRedraw();
	if (disabled)
	{
		if (GetViewport() != nullptr)
		{
			GetViewport()->CancelGuiInteraction(this);
		}
		else
		{
			CompletePress(false);
		}
	}
}

void BaseButton::SetToggleMode(bool toggleMode)
{
	if (m_toggleMode == toggleMode)
	{
		return;
	}

	m_toggleMode = toggleMode;
	if (!toggleMode && m_status.m_pressed)
	{
		m_status.m_pressed = false;
		QueueRedraw();
		auto callback = m_toggledCallback;
		if (callback)
		{
			callback(false);
		}

		return;
	}

	QueueRedraw();
}

void BaseButton::SetPressed(bool pressed)
{
	if (!m_toggleMode || m_status.m_pressed == pressed)
	{
		return;
	}

	m_status.m_pressed = pressed;
	QueueRedraw();
	auto callback = m_toggledCallback;
	if (callback)
	{
		callback(pressed);
	}
}

BaseButton::DrawMode BaseButton::GetDrawMode() const
{
	if (m_status.m_disabled)
	{
		return DrawMode::Disabled;
	}

	if (m_status.m_pressAttempt)
	{
		bool const down = m_status.m_pressingInside != m_status.m_pressed;
		return down ? DrawMode::Pressed : DrawMode::Normal;
	}

	if (m_status.m_pressed)
	{
		return m_status.m_hovering ? DrawMode::HoverPressed : DrawMode::Pressed;
	}

	return m_status.m_hovering ? DrawMode::Hover : DrawMode::Normal;
}

void BaseButton::SetPressedNoSignal(bool pressed)
{
	if (m_toggleMode && m_status.m_pressed != pressed)
	{
		m_status.m_pressed = pressed;
		QueueRedraw();
	}
}

void BaseButton::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_MouseEnter:
		m_status.m_hovering = true;
		QueueRedraw();
		break;
	case Notification_MouseExit:
		m_status.m_hovering = false;
		QueueRedraw();
		break;
	case Notification_FocusEnter:
		QueueRedraw();
		break;
	case Notification_FocusExit:
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		QueueRedraw();
		CompletePress(false);
		break;
	}
}

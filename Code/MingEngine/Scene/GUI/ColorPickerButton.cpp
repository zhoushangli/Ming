#include "MingEngine/Scene/GUI/ColorPickerButton.hpp"

#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#include <algorithm>

ColorPickerButton::ColorPickerButton()
{
	SetPressedCallback(
		[this]()
		{
			OpenPicker();
		});
}

ColorPickerButton::~ColorPickerButton()
{
	ClosePicker(false, false);
	if (auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->SetClosedCallback({});
		popup->SetKeyCallback({});
		popup->SetPointerCallback({});
		m_picker->SetColorChangedCallback({});
		m_picker->SetPickRequestedCallback({});
		m_picker->SetScreenColorReader({});
		popup->QueueFree();
	}
}

Vector2 ColorPickerButton::GetInternalMinimumSize() const
{
	Vector2 const minimum = Button::GetInternalMinimumSize();
	return Vector2(std::max(minimum.x, static_cast<float>(GetThemeConstant("row_height", "ColorPicker"))), minimum.y);
}

void ColorPickerButton::SetColorNoSignal(Color const& color)
{
	if (m_color == color)
	{
		return;
	}

	m_color = color;
	if (m_picker != nullptr)
	{
		m_picker->SetColorNoSignal(color);
	}

	QueueRedraw();
}

void ColorPickerButton::SetScreenColorReader(ColorPicker::ScreenColorReader reader)
{
	m_screenReader = std::move(reader);
	if (m_picker != nullptr)
	{
		m_picker->SetScreenColorReader(m_screenReader);
	}
}

ColorPicker* ColorPickerButton::GetPicker()
{
	if (m_picker != nullptr)
	{
		return m_picker;
	}

	if (GetViewport() == nullptr)
	{
		return nullptr;
	}

	auto* popup = MemNew<PopupPanel>();
	popup->SetSerializable(false);
	m_popupID = popup->GetObjectID();
	GetViewport()->AddNode(popup);
	m_scroll = MemNew<ScrollContainer>();
	popup->AddNode(m_scroll);
	m_picker = MemNew<ColorPicker>();
	m_scroll->AddNode(m_picker);
	m_picker->SetColorNoSignal(m_color);
	m_picker->SetScreenColorReader(m_screenReader);
	m_picker->SetColorChangedCallback(
		[this](Color const& color)
		{
			Preview(color);
		});
	m_picker->SetPickRequestedCallback(
		[this]()
		{
			StartScreenPicking();
		});
	popup->SetClosedCallback(
		[this]()
		{
			PopupClosed();
		});
	popup->SetKeyCallback(
		[this](InputEvent const& event)
		{
			HandleKey(event);
		});
	popup->SetPointerCallback(
		[this](InputEvent const& event)
		{
			return HandlePointer(event);
		});
	return m_picker;
}

void ColorPickerButton::PlacePopup()
{
	if (GetViewport() == nullptr || m_picker == nullptr)
	{
		return;
	}

	Vector2 const bounds(GetViewport()->GetOutputResolution());
	Vector2 const minimum = m_picker->GetCombinedMinimumSize();
	Vector2 const size(std::min(bounds.x, minimum.x), std::min(bounds.y, minimum.y));
	Vector2 const origin = GetGlobalPosition();
	float const   y      = origin.y + GetSize().y + size.y <= bounds.y ? origin.y + GetSize().y : origin.y - size.y;
	Vector2 const position(
		std::clamp(origin.x, 0.f, std::max(0.f, bounds.x - size.x)),
		std::clamp(y, 0.f, std::max(0.f, bounds.y - size.y)));
	auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID);
	if (popup == nullptr
		|| (m_scroll->GetLocalPosition() == position && m_scroll->GetSize() == size && popup->GetSize() == bounds))
	{
		return;
	}

	popup->UpdateSize(bounds);
	m_scroll->SetPosition(position);
	m_scroll->UpdateSize(size);
	GetSceneTree()->FlushContainerUpdate();
}

void ColorPickerButton::OpenPicker()
{
	if (m_editing || IsDisabled() || GetPicker() == nullptr)
	{
		return;
	}

	auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID);
	popup->Open(this);
	if (!popup->IsVisible())
	{
		return;
	}

	m_original = m_color;
	m_editing  = true;
	m_accept   = true;
	m_picker->SetOldColor(m_original);
	m_picker->SetColorNoSignal(m_color);
	m_scroll->SetVisible(true);
	PlacePopup();
	m_picker->GrabFocus();
	auto callback = m_started;
	if (callback)
	{
		callback();
	}
}

void ColorPickerButton::ClosePicker(bool commit, bool restoreFocus)
{
	if (!m_editing)
	{
		return;
	}

	m_accept = commit;
	m_picker->EndEditing(commit);
	if (auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->Close(restoreFocus);
	}
}

void ColorPickerButton::PopupClosed()
{
	if (!m_editing)
	{
		return;
	}

	auto*      popup  = ObjectDatabase::GetInstance<PopupPanel>(m_popupID);
	bool const accept = m_accept && popup != nullptr && !popup->WasInterrupted();
	m_picker->EndEditing(accept);
	m_picking = false;
	if (popup != nullptr)
	{
		popup->SetCursorOverride(CursorShape::Arrow);
	}

	m_editing = false;
	if (!accept)
	{
		SetColorNoSignal(m_original);
	}

	auto callback = m_ended;
	if (callback)
	{
		callback(accept);
	}
}

void ColorPickerButton::Preview(Color const& color)
{
	m_color = color;
	QueueRedraw();
	auto callback = m_changed;
	if (callback)
	{
		callback(color);
	}
}

void ColorPickerButton::HandleKey(InputEvent const& event)
{
	if (m_picking)
	{
		if (event.m_type == InputEventType::Key && event.m_pressed && event.m_keyCode == GLFW_KEY_ESCAPE)
		{
			FinishScreenPicking(false);
		}

		return;
	}

	if (m_picker->HandleKey(event))
	{
		return;
	}

	if (event.m_type == InputEventType::Key && event.m_pressed && event.m_keyCode == GLFW_KEY_ESCAPE)
	{
		ClosePicker(false);
	}
}

bool ColorPickerButton::HandlePointer(InputEvent const& event)
{
	if (!m_picking)
	{
		return m_scroll->IsPointInside(event.m_position - m_scroll->GetLocalPosition());
	}

	Vector2 const bounds(GetViewport()->GetOutputResolution());
	bool const    inside = event.m_position.x >= 0.f && event.m_position.y >= 0.f && event.m_position.x < bounds.x
						   && event.m_position.y < bounds.y;
	if (event.m_type == InputEventType::MouseButton && event.m_pressed
		&& event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
	{
		FinishScreenPicking(false);
		return true;
	}

	Color      sample;
	bool const valid = inside && m_picker->ReadScreenColor(event.m_position, sample);
	m_validSample    = valid;
	if (valid)
	{
		sample.a = 255;
		m_picker->SetColorNoSignal(sample);
		Preview(sample);
	}

	if (valid && event.m_type == InputEventType::MouseButton && event.m_pressed
		&& event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
	{
		FinishScreenPicking(true);
	}

	return true;
}

void ColorPickerButton::StartScreenPicking()
{
	if (!m_editing || !m_screenReader || m_picking)
	{
		return;
	}

	m_picker->EndEditing(true);
	m_beforePick  = m_color;
	m_picking     = true;
	m_validSample = false;
	m_scroll->SetVisible(false);
	if (auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->SetCursorOverride(CursorShape::Crosshair);
		popup->GrabFocus();
	}
}

void ColorPickerButton::FinishScreenPicking(bool accept)
{
	if (!accept || !m_validSample)
	{
		m_picker->SetColorNoSignal(m_beforePick);
		Preview(m_beforePick);
	}

	m_picking = false;
	if (auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->SetCursorOverride(CursorShape::Arrow);
	}

	m_scroll->SetVisible(true);
	PlacePopup();
	m_picker->GrabFocus();
}

void ColorPickerButton::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Process:
		if (m_editing)
		{
			if (!IsVisibleInHierarchy() || IsDisabled())
			{
				ClosePicker(false, false);
			}
			else
			{
				PlacePopup();
			}
		}
		break;
	case Notification_Draw:
	{
		AABB2 rect = GetLocalRect();
		rect.Shrink(std::max(1.f, GetThemeConstant("checker_size", "ColorPicker") * 0.5f));
		float const cell = static_cast<float>(std::max(1, GetThemeConstant("checker_size", "ColorPicker")));
		for (float y = rect.m_mins.y; y < rect.m_maxs.y; y += cell)
		{
			for (float x = rect.m_mins.x; x < rect.m_maxs.x; x += cell)
			{
				int const parity =
					static_cast<int>((x - rect.m_mins.x) / cell) + static_cast<int>((y - rect.m_mins.y) / cell);
				DrawRect(
					AABB2(x, y, std::min(x + cell, rect.m_maxs.x), std::min(y + cell, rect.m_maxs.y)),
					GetThemeColor(parity % 2 == 0 ? "checker_light" : "checker_dark", "ColorPicker"));
			}
		}

		DrawRect(rect, m_color);
		break;
	}
	case Notification_ExitTree:
	case Notification_GuiInputCancelled:
		ClosePicker(false, false);
		break;
	case Notification_VisibilityChanged:
		if (!IsVisibleInHierarchy())
		{
			ClosePicker(false, false);
		}
		break;
	}
}

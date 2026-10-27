#include "MingEngine/Scene/GUI/Button.hpp"

#include <algorithm>

Button::Button()
{
	m_label = MemNew<Label>();
	m_label->SetName("ButtonText");
	m_label->SetSerializable(false);
	m_label->SetHorizontalAlignment(HorizontalAlignment::Center);
	m_label->SetVerticalAlignment(VerticalAlignment::Center);
	AddNode(m_label);
}

Vector2 Button::GetInternalMinimumSize() const
{
	return m_label != nullptr ? m_label->GetCombinedMinimumSize() + m_padding * 2.f : m_padding * 2.f;
}

void Button::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	case Notification_Resized:
	case Notification_MinimumSizeChanged:
		if (m_label != nullptr)
		{
			Vector2 const size = GetSize() - m_padding * 2.f;
			m_label->SetPosition(m_padding);
			m_label->UpdateSize(Vector2(std::max(0.f, size.x), std::max(0.f, size.y)));
		}

		break;
	case Notification_Draw:
	{
		Color background(55, 65, 82);

		switch (GetDrawMode())
		{
		case DrawMode::Hover:
			background = Color(75, 95, 125);
			break;
		case DrawMode::Pressed:
			background = Color(35, 100, 165);
			break;
		case DrawMode::HoverPressed:
			background = Color(45, 125, 195);
			break;
		case DrawMode::Disabled:
			background = Color(38, 40, 46);
			break;
		default:
			break;
		}

		DrawRect(GetLocalRect(), HasFocus() ? Color(245, 210, 90) : Color(115, 130, 150));
		float const border = HasFocus() ? 3.f : 1.f;
		AABB2       inner  = GetLocalRect();
		inner.Shrink(border);
		DrawRect(inner, background);
		break;
	}
	}
}

#include "MingEngine/Scene/GUI/ColorRect.hpp"

void ColorRect::SetColor(Color const& color)
{
	if (m_color == color)
	{
		return;
	}

	m_color = color;
	QueueRedraw();
}

void ColorRect::OnNotification(int notification)
{
	if (notification == Notification_Draw)
	{
		DrawRect(GetLocalRect(), m_color);
	}
}

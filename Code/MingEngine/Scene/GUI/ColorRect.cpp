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
	switch (notification)
	{
	case Notification_Draw:
	{
		DrawRect(GetLocalRect(), m_color);
		break;
	}
	}
}

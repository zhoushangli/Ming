#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

class ColorRect : public Control
{
	MCLASS(ColorRect, Control);

public:
	static void BindMethods() {}

	void SetColor(Color const& color)
	{
		if (m_color == color)
		{
			return;
		}

		m_color = color;
		QueueRedraw();
	}

	Color GetColor() const { return m_color; }

protected:
	void OnNotification(int notification)
	{
		if (notification == Notification_Draw)
		{
			DrawRect(GetLocalRect(), m_color);
		}
	}

private:
	Color m_color = Color::White;
};
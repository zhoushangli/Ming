#include "MingEngine/Scene/GUI/MarginContainer.hpp"

Vector2 MarginContainer::GetInternalMinimumSize() const
{
	return Container::GetInternalMinimumSize() + Vector2(m_marginLeft + m_marginRight, m_marginTop + m_marginBottom);
}

void MarginContainer::SetMargins(float left, float top, float right, float bottom)
{
	left   = left < 0.0f ? 0.0f : left;
	top    = top < 0.0f ? 0.0f : top;
	right  = right < 0.0f ? 0.0f : right;
	bottom = bottom < 0.0f ? 0.0f : bottom;

	if (m_marginLeft == left && m_marginTop == top && m_marginRight == right && m_marginBottom == bottom)
	{
		return;
	}

	m_marginLeft   = left;
	m_marginTop    = top;
	m_marginRight  = right;
	m_marginBottom = bottom;
	PropagateMinimumSizeChanged();
}

void MarginContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_UpdateLayout:
	{
		Rect2 rect;
		rect.SetPosition(Vector2(m_marginLeft, m_marginTop));
		rect.SetSize(GetSize() - Vector2(m_marginLeft + m_marginRight, m_marginTop + m_marginBottom));

		for (Node* child : GetChildren())
		{
			if (Control* control = GetLayoutChild(child))
			{
				UpdateChildRect(control, rect);
			}
		}

		break;
	}
	}
}

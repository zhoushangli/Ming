#include "MingEngine/Scene/GUI/BoxContainer.hpp"

Vector2 BoxContainer::GetInternalMinimumSize() const
{
	Vector2 minimumSize = Vector2::Zero;
	bool    hasChild    = false;

	for (Node* child : GetChildren())
	{
		Control* control = GetLayoutChild(child);
		if (control == nullptr)
		{
			continue;
		}

		Vector2 const childMinimumSize = control->GetCombinedMinimumSize();
		float const   separation       = hasChild ? m_separation : 0.0f;
		if (m_vertical)
		{
			minimumSize.y += childMinimumSize.y + separation;
			minimumSize.x = minimumSize.x < childMinimumSize.x ? childMinimumSize.x : minimumSize.x;
		}
		else
		{
			minimumSize.x += childMinimumSize.x + separation;
			minimumSize.y = minimumSize.y < childMinimumSize.y ? childMinimumSize.y : minimumSize.y;
		}
		hasChild = true;
	}

	return minimumSize;
}

void BoxContainer::SetSeparation(float separation)
{
	separation = separation < 0.0f ? 0.0f : separation;
	if (m_separation == separation)
	{
		return;
	}

	m_separation = separation;
	PropagateMinimumSizeChanged();
}

void BoxContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_UpdateLayout:
	{
		// 1) Count children that expand along the main axis
		int expandCount = 0;
		for (Node* child : GetChildren())
		{
			Control* control = GetLayoutChild(child);
			if (control == nullptr)
			{
				continue;
			}

			int const flags = m_vertical ? control->GetVerticalSizeFlags() : control->GetHorizontalSizeFlags();

			if ((flags & Control::Size_Expand) != 0)
			{
				++expandCount;
			}
		}

		// 2) Divide positive remaining space along the main axis
		Vector2 const minimumSize     = GetInternalMinimumSize();
		float const   availableLength = m_vertical ? GetSize().y : GetSize().x;
		float const   minimumLength   = m_vertical ? minimumSize.y : minimumSize.x;
		float const   extraSpace      = availableLength - minimumLength;
		float const   extraPerChild   = expandCount > 0 && extraSpace > 0.0f ? extraSpace / (float)expandCount : 0.0f;

		// 3) Assign child rectangles
		float offset = 0.0f;
		for (Node* child : GetChildren())
		{
			Control* control = GetLayoutChild(child);
			if (control == nullptr)
			{
				continue;
			}

			Vector2 const childMinimumSize = control->GetCombinedMinimumSize();
			int const     flags  = m_vertical ? control->GetVerticalSizeFlags() : control->GetHorizontalSizeFlags();
			float         length = m_vertical ? childMinimumSize.y : childMinimumSize.x;
			if ((flags & Control::Size_Expand) != 0)
			{
				length += extraPerChild;
			}

			Rect2 rect;
			if (m_vertical)
			{
				rect.SetPosition(Vector2(0.0f, offset));
				rect.SetSize(Vector2(GetSize().x, length));
			}
			else
			{
				rect.SetPosition(Vector2(offset, 0.0f));
				rect.SetSize(Vector2(length, GetSize().y));
			}
			UpdateChildRect(control, rect);
			offset += length + m_separation;
		}
		break;
	}
	}
}

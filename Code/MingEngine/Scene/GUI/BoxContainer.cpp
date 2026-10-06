#include "MingEngine/Scene/GUI/BoxContainer.hpp"

Vector2 BoxContainer::GetMinimumSize() const
{
	Vector2 minimumSize = Vector2::Zero;
	bool hasChild = false;

	for (Node* child : GetChildren())
	{
		Control* control = GetLayoutChild(child);
		if (control == nullptr)
		{
			continue;
		}

		Vector2 const childMinimumSize = control->GetCombinedMinimumSize();
		float const separation = hasChild ? m_separation : 0.0f;
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
	if (notification != Notification_SortChildren)
	{
		return;
	}

	float offset = 0.0f;
	for (Node* child : GetChildren())
	{
		Control* control = GetLayoutChild(child);
		if (control == nullptr)
		{
			continue;
		}

		Vector2 const childMinimumSize = control->GetCombinedMinimumSize();
		Rect2 rect;
		if (m_vertical)
		{
			rect.SetPosition(Vector2(0.0f, offset));
			rect.SetSize(Vector2(GetSize().x, childMinimumSize.y));
			offset += childMinimumSize.y + m_separation;
		}
		else
		{
			rect.SetPosition(Vector2(offset, 0.0f));
			rect.SetSize(Vector2(childMinimumSize.x, GetSize().y));
			offset += childMinimumSize.x + m_separation;
		}
		FitChildInRect(control, rect);
	}
}

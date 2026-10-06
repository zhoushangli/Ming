#include "MingEngine/Scene/GUI/Control.hpp"

Vector2 Control::GetMinimumSize() const { return Vector2::Zero; }

Vector2 Control::GetCombinedMinimumSize() const
{
	Vector2 const minimumSize = GetMinimumSize();

	return Vector2(
		minimumSize.x < m_customMinimumSize.x ? m_customMinimumSize.x : minimumSize.x,
		minimumSize.y < m_customMinimumSize.y ? m_customMinimumSize.y : minimumSize.y);
}

void Control::SetPosition(Vector2 const& position)
{
	m_position = position;
	SyncPosition();
}

void Control::SetSize(Vector2 const& size)
{
	Vector2 const minimumSize = GetCombinedMinimumSize();

	Vector2 const newSize(
		size.x < minimumSize.x ? minimumSize.x : size.x,
		size.y < minimumSize.y ? minimumSize.y : size.y);

	if (m_size == newSize)
	{
		return;
	}

	m_size = newSize;
	Notification(Notification_Resized);
	QueueRedraw();
}

void Control::SetCustomMinimumSize(Vector2 const& size)
{
	Vector2 const newMinimumSize(size.x < 0.0f ? 0.0f : size.x, size.y < 0.0f ? 0.0f : size.y);

	if (m_customMinimumSize == newMinimumSize)
	{
		return;
	}

	m_customMinimumSize = newMinimumSize;
	PropagateMinimumSizeChanged();
}

void Control::PropagateMinimumSizeChanged()
{
	SetSize(m_size);

	Notification(Notification_MinimumSizeChanged);

	if (Control* parent = dynamic_cast<Control*>(GetParent()))
	{
		parent->PropagateMinimumSizeChanged();
	}
}

void Control::OnNotification(int notification)
{
	if (notification == Notification_VisibilityChanged)
	{
		if (Control* parent = dynamic_cast<Control*>(GetParent()))
		{
			parent->PropagateMinimumSizeChanged();
		}
	}
}
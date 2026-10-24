#include "MingEngine/Scene/GUI/Control.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"

void Control::UpdateSize(Vector2 const& newSize)
{
	Vector2 const minimumSize = GetCombinedMinimumSize();

	Vector2 const updatedSize(
		newSize.x < minimumSize.x ? minimumSize.x : newSize.x,
		newSize.y < minimumSize.y ? minimumSize.y : newSize.y);

	if (m_size == updatedSize)
	{
		return;
	}

	SetSize(updatedSize);

	QueueRedraw();
}

Vector2 Control::GetInternalMinimumSize() const { return Vector2::Zero; }

Vector2 Control::GetCombinedMinimumSize() const
{
	Vector2 const minimumSize = GetInternalMinimumSize();

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
	m_size = size;
	Notification(Notification_Resized);
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

void Control::SetHorizontalSizeFlags(int flags)
{
	if (m_horizontalSizeFlags == flags)
	{
		return;
	}

	m_horizontalSizeFlags = flags;

	if (Container* parent = dynamic_cast<Container*>(GetParent()))
	{
		parent->MarkDirty();
	}
}

void Control::SetVerticalSizeFlags(int flags)
{
	if (m_verticalSizeFlags == flags)
	{
		return;
	}

	m_verticalSizeFlags = flags;

	if (Container* parent = dynamic_cast<Container*>(GetParent()))
	{
		parent->MarkDirty();
	}
}

void Control::PropagateMinimumSizeChanged()
{
	UpdateSize(m_size);

	// Notification spread the message along the inherit chain, from child to parent
	Notification(Notification_MinimumSizeChanged);

	if (Control* parent = dynamic_cast<Control*>(GetParent()))
	{
		parent->PropagateMinimumSizeChanged();
	}
}

void Control::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	{
		if (m_data.m_viewport != nullptr && GetParentItem() == nullptr)
		{
			m_data.m_viewport->AddRootControl(this);
		}
		break;
	}
	case Notification_ExitTree:
	{
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->RemoveRootControl(this);
		}
		break;
	}
	case Notification_VisibilityChanged:
	{
		if (Control* parent = dynamic_cast<Control*>(GetParent()))
		{
			parent->PropagateMinimumSizeChanged();
		}
		break;
	}
	}
}

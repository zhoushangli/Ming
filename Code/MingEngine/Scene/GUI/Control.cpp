#include "MingEngine/Scene/GUI/Control.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"

bool Control::IsPointInside(Vector2 const& localPosition) const
{
	return m_size.x > 0.f && m_size.y > 0.f && localPosition.x >= 0.f && localPosition.y >= 0.f
		   && localPosition.x < m_size.x && localPosition.y < m_size.y;
}

bool Control::GuiInput([[maybe_unused]] InputEvent const& event)
{
	return false;
}

bool Control::GetDragData(Vector2 const& position, GuiDragData& data)
{
	auto callback = m_dragSource;
	return callback && callback(position, data);
}

bool Control::CanDropData(Vector2 const& position, GuiDragData const& data) const
{
	auto callback = m_dropCheck;
	return callback && callback(position, data);
}

void Control::DropData(Vector2 const& position, GuiDragData const& data)
{
	auto callback = m_drop;
	if (callback)
	{
		callback(position, data);
	}
}

void Control::DragHover(Vector2 const& position, GuiDragData const& data, bool accepted)
{
	auto callback = m_dragHover;
	if (callback)
	{
		callback(position, data, accepted);
	}
}

void Control::SetDragPreview(Control* preview)
{
	if (GetViewport() != nullptr)
	{
		GetViewport()->SetGuiDragPreview(preview);
	}
}

void Control::SetDragCallbacks(
	std::function<bool(Vector2 const&, GuiDragData&)> source,
	std::function<bool(Vector2 const&, GuiDragData const&)> check,
	std::function<void(Vector2 const&, GuiDragData const&)> drop)
{
	m_dragSource = std::move(source);
	m_dropCheck = std::move(check);
	m_drop = std::move(drop);
}

void Control::SetDragHoverCallback(std::function<void(Vector2 const&, GuiDragData const&, bool)> callback)
{
	m_dragHover = std::move(callback);
}

void Control::SetMouseFilter(MouseFilter filter)
{
	if (m_mouseFilter == filter)
	{
		return;
	}

	m_mouseFilter = filter;
	if (filter == MouseFilter::Ignore && GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this, false);
	}
}

void Control::SetFocusMode(FocusMode mode)
{
	m_focusMode = mode;
	if (mode == FocusMode::None)
	{
		ReleaseFocus();
	}
}

void Control::GrabFocus()
{
	if (GetViewport() != nullptr)
	{
		GetViewport()->SetGuiFocus(this);
	}
}

void Control::ReleaseFocus()
{
	if (HasFocus())
	{
		GetViewport()->SetGuiFocus(nullptr);
	}
}

bool Control::HasFocus() const
{
	return GetViewport() != nullptr && GetViewport()->HasGuiFocus(this);
}

void Control::SetClipContents(bool enabled)
{
	m_clipContents = enabled;
	SetCanvasClip(enabled, m_size);
}

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

Vector2 Control::GetInternalMinimumSize() const
{
	return Vector2::Zero;
}

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
	SetCanvasClip(m_clipContents, m_size);
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
	case Notification_DragHoverExit:
	case Notification_DragEnd:
		DragHover(Vector2(-1.f, -1.f), {}, false);
		break;
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
			m_data.m_viewport->CancelGuiInteraction(this);
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

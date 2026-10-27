#include "MingEngine/Scene/GUI/Container.hpp"

#include "MingEngine/Scene/Core/SceneTree.hpp"

Control* Container::GetLayoutChild(Node* child) const
{
	Control* control = dynamic_cast<Control*>(child);
	if (control == nullptr || control->GetParent() != this || !control->IsVisible())
	{
		return nullptr;
	}

	return control;
}

Vector2 Container::GetInternalMinimumSize() const
{
	Vector2 minimumSize = Vector2::Zero;

	for (Node* child : GetChildren())
	{
		Control* control = GetLayoutChild(child);
		if (control == nullptr)
		{
			continue;
		}

		Vector2 const childMinimumSize = control->GetCombinedMinimumSize();

		if (minimumSize.x < childMinimumSize.x)
		{
			minimumSize.x = childMinimumSize.x;
		}

		if (minimumSize.y < childMinimumSize.y)
		{
			minimumSize.y = childMinimumSize.y;
		}
	}

	return minimumSize;
}

void Container::MarkDirty()
{
	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr || m_data.m_isPendingDestroy || m_updatePending)
	{
		return;
	}

	m_updatePending = true;
	sceneTree->QueueContainerSort(GetObjectID());
}

void Container::UpdateLayout()
{
	if (!m_updatePending)
	{
		return;
	}

	// 1) Consume the current request
	m_updatePending = false;

	if (GetSceneTree() == nullptr || m_data.m_isPendingDestroy)
	{
		return;
	}

	// 2) Let the concrete container assign child rectangles
	Notification(Notification_UpdateLayout);
}

void Container::UpdateChildRect(Control* child, Rect2 const& rect)
{
	if (child == nullptr || child->GetParent() != this)
	{
		return;
	}

	Vector2 const minimumSize     = child->GetCombinedMinimumSize();
	Vector2 const allocatedSize   = rect.GetSize();
	Vector2       position        = rect.GetPosition();
	Vector2       size            = allocatedSize;
	int const     horizontalFlags = child->GetHorizontalSizeFlags();
	int const     verticalFlags   = child->GetVerticalSizeFlags();

	// 1) Apply horizontal size and alignment
	if ((horizontalFlags & Control::Size_Fill) == 0)
	{
		size.x = minimumSize.x;
		if ((horizontalFlags & Control::Size_ShrinkEnd) != 0)
		{
			position.x += allocatedSize.x - size.x;
		}
		else if ((horizontalFlags & Control::Size_ShrinkCenter) != 0)
		{
			position.x += (allocatedSize.x - size.x) * 0.5f;
		}
	}

	// 2) Apply vertical size and alignment
	if ((verticalFlags & Control::Size_Fill) == 0)
	{
		size.y = minimumSize.y;
		if ((verticalFlags & Control::Size_ShrinkEnd) != 0)
		{
			position.y += allocatedSize.y - size.y;
		}
		else if ((verticalFlags & Control::Size_ShrinkCenter) != 0)
		{
			position.y += (allocatedSize.y - size.y) * 0.5f;
		}
	}

	// 3) Apply the child rectangle
	child->SetPosition(position);
	child->UpdateSize(size);
}

void Container::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	case Notification_Resized:
	case Notification_MinimumSizeChanged:
	case Notification_VisibilityChanged:
	{
		MarkDirty();
		break;
	}

	case Notification_ChildrenChanged:
	{
		// Not only dirty, but also need to recalculate the minimum size and inform the parent
		PropagateMinimumSizeChanged();
		break;
	}

	case Notification_ExitTree:
	{
		m_updatePending = false;
		break;
	}
	}
}

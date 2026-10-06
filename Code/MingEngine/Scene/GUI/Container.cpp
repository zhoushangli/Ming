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

Vector2 Container::GetMinimumSize() const
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

void Container::QueueSort()
{
	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr || m_data.m_isPendingDestroy || m_sortPending)
	{
		return;
	}

	m_sortPending = true;
	sceneTree->QueueContainerSort(GetObjectID());
}

void Container::SortChildren()
{
	if (!m_sortPending)
	{
		return;
	}

	// 1) Consume the current request
	m_sortPending = false;

	if (GetSceneTree() == nullptr || m_data.m_isPendingDestroy)
	{
		return;
	}

	// 2) Let the concrete container assign child rectangles
	Notification(Notification_SortChildren);
}

void Container::FitChildInRect(Control* child, Rect2 const& rect)
{
	if (child == nullptr || child->GetParent() != this)
	{
		return;
	}

	child->SetPosition(rect.GetPosition());
	child->SetSize(rect.GetSize());
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
		QueueSort();
		break;
	}

	case Notification_ChildrenChanged:
	{
		PropagateMinimumSizeChanged();
		break;
	}

	case Notification_ExitTree:
	{
		m_sortPending = false;
		break;
	}
	}
}

#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

// For Container, when its children or Viewport changed, it will mark itself dirty
// and in the next frame, the scene tree will call UpdateLayout to update the container
// This is to avoid the container update layout multiple times in a single frame

// For Container, it will actually arrange its children the Notification_UpdateLayout notification, so the container can
// arrange its children in any way it wants
class Container : public Control
{
	MCLASS(Container, Control);

	friend class SceneTree;

public:
	enum
	{
		Notification_UpdateLayout = 34
	};

public:
	static void BindMethods() {}

	Container() { SetMouseFilter(MouseFilter::Pass); }

	Vector2 GetInternalMinimumSize() const override;

	void MarkDirty();
	void UpdateChildRect(Control* child, Rect2 const& rect);

protected:
	Control* GetLayoutChild(Node* child) const;

	void OnNotification(int notification);

private:
	void UpdateLayout();

	// Weather it is waiting for the SceneTree to call UpdateLayout
	// if it is true, then we do not need to call UpdateLayout again
	bool m_updatePending = false;
};

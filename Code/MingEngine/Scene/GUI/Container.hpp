#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

class Container : public Control
{
	MCLASS(Container, Control);

	friend class SceneTree;

public:
	enum
	{
		Notification_SortChildren = 34
	};

public:
	static void BindMethods() {}

	Vector2 GetInternalMinimumSize() const override;

	void MarkDirty();
	void FitChildInRect(Control* child, Rect2 const& rect);

protected:
	Control* GetLayoutChild(Node* child) const;

	void OnNotification(int notification);

private:
	void SortChildren();

	// Weather it is waiting for the SceneTree to call SortChildren
	// if it is true, then we do not need to call SortChildren again
	bool m_sortPending = false;
};
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

	Vector2 GetMinimumSize() const override;

	void QueueSort();
	void FitChildInRect(Control* child, Rect2 const& rect);

protected:
	Control* GetLayoutChild(Node* child) const;

	void OnNotification(int notification);

private:
	void SortChildren();

	bool m_sortPending = false;
};
#pragma once

#include "MingEngine/Scene/Core/CanvasItem.hpp"

// Control inherits from CanvasItem, which means Control can call all these render functions
// but Control has its own unique features, like size, position, minimum size, etc.
// this is because Control is a UI element, which means it will take the whole screen as its target
// which is very different from node 2d
class Control : public CanvasItem
{
	MCLASS(Control, CanvasItem);

public:
	enum
	{
		Notification_Resized            = 31,
		Notification_MinimumSizeChanged = 32
	};

public:
	static void BindMethods() {}

	void UpdateSize(Vector2 const& newSize);

	Vector2 GetSize() const { return m_size; }
	AABB2   GetLocalRect() const { return AABB2(Vector2::Zero, m_size); }
	Vector2 GetLocalPosition() const override { return m_position; }

	virtual Vector2 GetInternalMinimumSize() const;
	Vector2         GetCustomMinimumSize() const { return m_customMinimumSize; }
	Vector2         GetCombinedMinimumSize() const;

	void SetPosition(Vector2 const& position);

	void SetCustomMinimumSize(Vector2 const& size);

	// Propagate is spread the message along tree structure
	void PropagateMinimumSizeChanged();

protected:
	void OnNotification(int notification);

private:
	void SetSize(Vector2 const& size);

private:
	Vector2 m_position          = Vector2::Zero;
	Vector2 m_size              = Vector2::Zero;
	Vector2 m_customMinimumSize = Vector2::Zero;
};

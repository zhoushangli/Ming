#pragma once

#include "MingEngine/Scene/Core/CanvasItem.hpp"

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

	Vector2 GetSize() const { return m_size; }
	AABB2   GetLocalRect() const { return AABB2(Vector2::Zero, m_size); }
	Vector2 GetLocalPosition() const override { return m_position; }

	virtual Vector2 GetMinimumSize() const;
	Vector2         GetCombinedMinimumSize() const;
	Vector2         GetCustomMinimumSize() const { return m_customMinimumSize; }

	void SetPosition(Vector2 const& position);
	void SetSize(Vector2 const& size);
	void SetCustomMinimumSize(Vector2 const& size);

	void PropagateMinimumSizeChanged();

protected:
	void OnNotification(int notification);

private:
	Vector2 m_position          = Vector2::Zero;
	Vector2 m_size              = Vector2::Zero;
	Vector2 m_customMinimumSize = Vector2::Zero;
};

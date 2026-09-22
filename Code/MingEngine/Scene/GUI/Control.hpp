#pragma once

#include "MingEngine/Scene/Core/CanvasItem.hpp"

class Control : public CanvasItem
{
	MCLASS(Control, CanvasItem);

public:
	static void BindMethods() {}

	Vector2 GetLocalPosition() const override { return m_position; }

	void SetPosition(Vector2 const& position) { m_position = position; }

	Vector2 GetSize() const { return m_size; }

	void SetSize(Vector2 const& size)
	{
		Vector2 newSize(size.x < 0.0f ? 0.0f : size.x, size.y < 0.0f ? 0.0f : size.y);

		if (m_size == newSize)
		{
			return;
		}

		m_size = newSize;
		QueueRedraw();
	}

	AABB2 GetLocalRect() const { return AABB2(Vector2::Zero, m_size); }

private:
	Vector2 m_position = Vector2::Zero;
	Vector2 m_size     = Vector2::Zero;
};
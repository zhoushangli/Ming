#pragma once

#include "MingEngine/Scene/Core/CanvasItem.hpp"

class Control : public CanvasItem
{
	MCLASS(Control, CanvasItem);

public:
	static void BindMethods() {}

	Vector2 GetSize() const { return m_size; }
	AABB2   GetLocalRect() const { return AABB2(Vector2::Zero, m_size); }
	Vector2 GetLocalPosition() const override { return m_position; }

	void SetPosition(Vector2 const& position);
	void SetSize(Vector2 const& size);

private:
	Vector2 m_position = Vector2::Zero;
	Vector2 m_size     = Vector2::Zero;
};

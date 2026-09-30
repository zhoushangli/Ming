#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

struct Rect2
{
public:
	Vector2 const& GetPosition() const { return m_position; }
	Vector2 const& GetSize() const { return m_size; }
	float          GetArea() const { return m_size.x * m_size.y; }
	Vector2        GetCenter() const { return m_position + m_size * 0.5f; }

	void SetPosition(Vector2 const& position) { m_position = position; }
	void SetSize(Vector2 const& size) { m_size = size; }

public:
	// Rect is mainly for UI
	// So the position is the top-left corner, and the size is width and height
	Vector2 m_position = Vector2::Zero;
	Vector2 m_size     = Vector2::Zero;
};

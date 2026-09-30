#include "MingEngine/Scene/GUI/Control.hpp"

void Control::SetPosition(Vector2 const& position)
{
	m_position = position;
	SyncPosition();
}

void Control::SetSize(const Vector2& size)
{
	Vector2 newSize(size.x < 0.0f ? 0.0f : size.x, size.y < 0.0f ? 0.0f : size.y);

	if (m_size == newSize)
	{
		return;
	}

	m_size = newSize;
	QueueRedraw();
}
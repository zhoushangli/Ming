#include "MingEngine/Scene/GUI/TextureRect.hpp"

#include "MingEngine/Core/Math/Rect2.hpp"

void TextureRect::SetTexture(Ref<TextureResource> const& texture)
{
	if (m_texture == texture)
	{
		return;
	}

	m_texture = texture;
	QueueRedraw();
}

void TextureRect::SetHorizontalFlip(bool horizontalFlip)
{
	if (m_horizontalFlip == horizontalFlip)
	{
		return;
	}

	m_horizontalFlip = horizontalFlip;
	QueueRedraw();
}

void TextureRect::SetVerticalFlip(bool verticalFlip)
{
	if (m_verticalFlip == verticalFlip)
	{
		return;
	}

	m_verticalFlip = verticalFlip;
	QueueRedraw();
}

void TextureRect::SetExpandMode(ExpandMode expandMode)
{
	if (m_expandMode == expandMode)
	{
		return;
	}

	m_expandMode = expandMode;
	QueueRedraw();
}

void TextureRect::SetStretchMode(StretchMode stretchMode)
{
	if (m_stretchMode == stretchMode)
	{
		return;
	}

	m_stretchMode = stretchMode;
	QueueRedraw();
}

void TextureRect::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
	{
		if (!m_texture.IsValid())
		{
			return;
		}

		Vector2 size;
		Vector2 offset;
        Rect2 rect;

		break;
	}
	}
}

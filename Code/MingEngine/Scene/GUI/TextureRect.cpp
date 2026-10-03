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
		if (m_texture->GetDimensions().x <= 0 || m_texture->GetDimensions().y <= 0 ||
			GetSize().x <= 0.0f || GetSize().y <= 0.0f)
		{
			return;
		}

		Vector2 size;
		Vector2 offset;
		Rect2   rect;
		bool    isTiling = false;

		switch (m_stretchMode)
		{
		case StretchMode::Scale:
		{
			size = GetSize();
			break;
		}
		case StretchMode::Tile:
		{
			size     = GetSize();
			isTiling = true;
			break;
		}
		case StretchMode::Keep:
		{
			size = (Vector2)m_texture->GetDimensions();
			break;
		}
		case StretchMode::KeepCentered:
		{
			size   = (Vector2)m_texture->GetDimensions();
			offset = (GetSize() - size) * 0.5f;
			break;
		}
		case StretchMode::KeepAspect:
		{
			Vector2 textureSize  = (Vector2)m_texture->GetDimensions();
			float   textureRatio = textureSize.x / textureSize.y;
			float   rectRatio    = GetSize().x / GetSize().y;

			if (textureRatio > rectRatio)
			{
				size.x = GetSize().x;
				size.y = size.x / textureRatio;
			}
			else
			{
				size.y = GetSize().y;
				size.x = size.y * textureRatio;
			}
			break;
		}
		case StretchMode::KeepAspectCentered:
		{
			Vector2 textureSize  = (Vector2)m_texture->GetDimensions();
			float   textureRatio = textureSize.x / textureSize.y;
			float   rectRatio    = GetSize().x / GetSize().y;

			if (textureRatio > rectRatio)
			{
				size.x = GetSize().x;
				size.y = size.x / textureRatio;
			}
			else
			{
				size.y = GetSize().y;
				size.x = size.y * textureRatio;
			}

			offset = (GetSize() - size) * 0.5f;
			break;
		}
		case StretchMode::KeepAspectCovered:
		{
			Vector2 textureSize  = (Vector2)m_texture->GetDimensions();
			float   textureRatio = textureSize.x / textureSize.y;
			float   rectRatio    = GetSize().x / GetSize().y;

			if (textureRatio > rectRatio)
			{
				size.y = GetSize().y;
				size.x = size.y * textureRatio;
			}
			else
			{
				size.x = GetSize().x;
				size.y = size.x / textureRatio;
			}

			offset = (GetSize() - size) * 0.5f;
			break;
		}
		}

		if (m_horizontalFlip)
		{
			offset.x += size.x;
			size.x = -size.x;
		}
		if (m_verticalFlip)
		{
			offset.y += size.y;
			size.y = -size.y;
		}

		rect.m_position = offset;
		rect.m_size = size;
		DrawTextureRect(m_texture, rect, isTiling);
	}
	}
}

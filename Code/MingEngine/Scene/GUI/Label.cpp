#include "MingEngine/Scene/GUI/Label.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"

void Label::SetText(String const& text)
{
	if (m_text == text)
	{
		return;
	}

	m_text = text;
	QueueRedraw();
}

void Label::SetFont(Ref<FontResource> const& font)
{
	if (m_font == font)
	{
		return;
	}

	m_font = font;
	QueueRedraw();
}

void Label::SetFontSize(int fontSize)
{
	if (fontSize <= 0 || m_fontSize == fontSize)
	{
		return;
	}

	m_fontSize = fontSize;
	QueueRedraw();
}

void Label::SetHorizontalAlignment(HorizontalAlignment alignment)
{
	if (m_horizontalAlignment == alignment)
	{
		return;
	}

	m_horizontalAlignment = alignment;
	QueueRedraw();
}

void Label::SetVerticalAlignment(VerticalAlignment alignment)
{
	if (m_verticalAlignment == alignment)
	{
		return;
	}

	m_verticalAlignment = alignment;
	QueueRedraw();
}

void Label::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
	{
		if (!m_font.IsValid() || !m_font->GetFontRID().IsValid() || m_text.IsEmpty() || g_engine == nullptr
			|| g_engine->m_textServer == nullptr)
		{
			return;
		}

		TextServer*    textServer = g_engine->m_textServer;
		RID const      fontRID    = m_font->GetFontRID();
		TextLine const line       = textServer->GetTextLine(fontRID, m_text, static_cast<float>(m_fontSize));
		float const    baselineY  = line.m_config.m_ascent;

		// 1) Get alignment offsets
		float offsetX = 0.0f;
		float offsetY = 0.0f;

		Vector2 const rectTextLine =
			Vector2(line.m_width, std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent));
		Vector2 const rectSize = GetSize();
		Vector2 const rectDiff = rectSize - rectTextLine;

		switch (m_horizontalAlignment)
		{
		case HorizontalAlignment::Left:
			offsetX = 0.0f;
			break;
		case HorizontalAlignment::Center:
			offsetX = rectDiff.x * 0.5f;
			break;
		case HorizontalAlignment::Right:
			offsetX = rectDiff.x;
			break;
		// case HorizontalAlignment::Fill:
		// 	offsetX = 0.0f;
		// 	break;
		default:
			offsetX = 0.0f;
			break;
		}

		switch (m_verticalAlignment)
		{
		case VerticalAlignment::Top:
			offsetY = 0.0f;
			break;
		case VerticalAlignment::Center:
			offsetY = rectDiff.y * 0.5f;
			break;
		case VerticalAlignment::Bottom:
			offsetY = rectDiff.y;
			break;
		// case VerticalAlignment::Fill:
		// 	offsetY = 0.0f;
		// 	break;
		default:
			offsetY = 0.0f;
			break;
		}

		// 2) Draw each glyph in the line
		for (TextLineGlyph const& glyph : line.m_glyphs)
		{
			GlyphTexture const* glyphTexture =
				textServer->GetOrCreateGlyphTexture(fontRID, glyph.m_glyphIndex, m_fontSize);
			if (glyphTexture == nullptr || !glyphTexture->m_texture.IsValid())
			{
				continue;
			}

			GlyphImage const& bitmap = glyphTexture->m_bitmap;
			Vector2 const     glyphSize(static_cast<float>(bitmap.m_width), static_cast<float>(bitmap.m_height));

			Rect2 rect;
			rect.m_position = Vector2(
				glyph.m_posX + static_cast<float>(bitmap.m_offsetX) + offsetX,
				baselineY + static_cast<float>(bitmap.m_offsetY) + offsetY);
			rect.m_size = glyphSize;

			Rect2 sourceRect;
			sourceRect.m_position = Vector2::Zero;
			sourceRect.m_size     = glyphSize;

			DrawTextureRectRegion(glyphTexture->m_texture, rect, sourceRect);
		}

		break;
	}
	}
}

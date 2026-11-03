#include "MingEngine/Scene/GUI/Label.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include <algorithm>
#include <cmath>

Vector2 Label::GetInternalMinimumSize() const
{
	EnsureTextLayout();
	return m_minimum;
}

void Label::EnsureTextLayout() const
{
	RID const   font  = m_themeCache.font.IsValid() ? m_themeCache.font->GetFontRID() : RID::Invalid;
	float const width = GetSize().x;
	if (!m_layoutDirty && m_layoutFont == font && m_layoutFontSize == m_themeCache.font_size
		&& m_layoutLineSpacing == m_themeCache.line_spacing && m_layoutWidth == width)
	{
		return;
	}

	m_layoutDirty       = false;
	m_layoutFont        = font;
	m_layoutFontSize    = m_themeCache.font_size;
	m_layoutLineSpacing = m_themeCache.line_spacing;
	m_layoutWidth       = width;
	m_minimum           = Vector2::Zero;
	m_textHeight        = 0.f;
	m_lines.clear();
	if (!font.IsValid() || g_engine == nullptr || g_engine->m_textServer == nullptr)
	{
		return;
	}

	TextServer* server = g_engine->m_textServer;
	m_lines            = server->GetTextLines(font, m_text, static_cast<float>(m_layoutFontSize), width, m_wrap);
	for (TextLine& line : m_lines)
	{
		m_minimum.x        = std::max(m_minimum.x, line.m_width);
		float const height = std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent);
		if (m_textHeight > 0.f)
		{
			m_textHeight += std::max(0.f, line.m_config.m_lineGap) + std::max(0, m_layoutLineSpacing);
		}

		m_textHeight += height;
		line = server->TrimTextLine(font, line, static_cast<float>(m_layoutFontSize), width, m_overrun);
	}

	if (m_clipText || m_overrun != TextOverrunBehavior::TrimNothing || m_wrap != AutowrapMode::Off)
	{
		m_minimum.x = 1.f;
	}

	m_minimum.y = m_textHeight;
}

void Label::UpdateTextLayout()
{
	if (m_updatingLayout)
	{
		return;
	}

	m_updatingLayout       = true;
	Vector2 const previous = m_minimum;
	EnsureTextLayout();
	if (previous != m_minimum)
	{
		PropagateMinimumSizeChanged();
	}

	m_updatingLayout = false;
	QueueRedraw();
}

void Label::SetText(String const& text)
{
	if (m_text == text)
	{
		return;
	}

	m_text        = text;
	m_layoutDirty = true;
	UpdateTextLayout();
}

void Label::SetClipText(bool enabled)
{
	if (m_clipText == enabled)
	{
		return;
	}

	m_clipText    = enabled;
	m_layoutDirty = true;
	UpdateTextLayout();
}

void Label::SetTextOverrunBehavior(TextOverrunBehavior behavior)
{
	if (m_overrun == behavior)
	{
		return;
	}

	m_overrun     = behavior;
	m_layoutDirty = true;
	UpdateTextLayout();
}

void Label::SetAutowrapMode(AutowrapMode mode)
{
	if (m_wrap == mode)
	{
		return;
	}

	m_wrap        = mode;
	m_layoutDirty = true;
	UpdateTextLayout();
}

void Label::SetFont(Ref<FontResource> const& font)
{
	AddThemeFontOverride("font", font);
}

void Label::SetFontSize(int fontSize)
{
	AddThemeFontSizeOverride("font_size", fontSize);
}

void Label::SetFontColor(Color const& color)
{
	AddThemeColorOverride("font_color", color);
}

void Label::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Font, Label, font);
	BIND_THEME_ITEM(Theme::DataType::FontSize, Label, font_size);
	BIND_THEME_ITEM(Theme::DataType::Color, Label, font_color);
	BIND_THEME_ITEM(Theme::DataType::Constant, Label, line_spacing);
}

void Label::OnThemeChanged()
{
	UpdateTextLayout();
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

void Label::DrawText()
{
	EnsureTextLayout();
	if (m_lines.empty() || g_engine == nullptr || g_engine->m_textServer == nullptr)
	{
		return;
	}

	Vector2 const size = GetSize();
	float         y    = 0.f;
	switch (m_verticalAlignment)
	{
	case VerticalAlignment::Center:
		y = (size.y - m_textHeight) * 0.5f;
		break;
	case VerticalAlignment::Bottom:
		y = size.y - m_textHeight;
		break;
	default:
		break;
	}

	for (TextLine const& line : m_lines)
	{
		float x = 0.f;
		switch (m_horizontalAlignment)
		{
		case HorizontalAlignment::Center:
			x = (size.x - line.m_width) * 0.5f;
			break;
		case HorizontalAlignment::Right:
			x = size.x - line.m_width;
			break;
		default:
			break;
		}

		for (TextLineGlyph const& glyph : line.m_glyphs)
		{
			GlyphTexture const* texture =
				g_engine->m_textServer->GetOrCreateGlyphTexture(m_layoutFont, glyph.m_glyphIndex, m_layoutFontSize);
			if (texture == nullptr || !texture->m_texture.IsValid())
			{
				continue;
			}

			GlyphImage const& bitmap = texture->m_bitmap;
			Vector2 const position(x + glyph.m_posX + bitmap.m_offsetX, y + line.m_config.m_ascent + bitmap.m_offsetY);
			Vector2 const glyphSize(static_cast<float>(bitmap.m_width), static_cast<float>(bitmap.m_height));
			Vector2 const start = m_clipText ? Vector2(std::max(0.f, position.x), std::max(0.f, position.y)) : position;
			Vector2 const end =
				m_clipText
					? Vector2(std::min(size.x, position.x + glyphSize.x), std::min(size.y, position.y + glyphSize.y))
					: position + glyphSize;
			if (end.x <= start.x || end.y <= start.y)
			{
				continue;
			}

			Rect2 target;
			target.m_position = start;
			target.m_size     = end - start;
			Rect2 source;
			source.m_position = start - position;
			source.m_size     = target.m_size;
			DrawTextureRectRegion(texture->m_texture, target, source, m_themeCache.font_color);
		}

		y += std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent)
			 + std::max(0.f, line.m_config.m_lineGap) + std::max(0, m_layoutLineSpacing);
	}
}

void Label::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	case Notification_Resized:
		UpdateTextLayout();
		break;
	case Notification_Draw:
		DrawText();
		break;
	}
}

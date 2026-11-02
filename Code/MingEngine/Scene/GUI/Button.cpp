#include "MingEngine/Scene/GUI/Button.hpp"

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"

#include <cmath>
#include <algorithm>
#include <initializer_list>

Button::Button() = default;

void Button::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Button, normal);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Button, hover);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Button, pressed);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Button, hover_pressed);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Button, disabled);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, Button, focus);
	BIND_THEME_ITEM(Theme::DataType::Font, Button, font);
	BIND_THEME_ITEM(Theme::DataType::Icon, Button, icon);
	BIND_THEME_ITEM(Theme::DataType::Constant, Button, icon_size);
	BIND_THEME_ITEM(Theme::DataType::Constant, Button, icon_separation);
	BIND_THEME_ITEM(Theme::DataType::FontSize, Button, font_size);
	BIND_THEME_ITEM(Theme::DataType::Constant, Button, minimum_height);
	BIND_THEME_ITEM(Theme::DataType::Color, Button, font_color);
	BIND_THEME_ITEM(Theme::DataType::Color, Button, font_hover_color);
	BIND_THEME_ITEM(Theme::DataType::Color, Button, font_pressed_color);
	BIND_THEME_ITEM(Theme::DataType::Color, Button, font_hover_pressed_color);
	BIND_THEME_ITEM(Theme::DataType::Color, Button, font_disabled_color);
}

AABB2 Button::GetTextRect() const
{
	float margins[4] = {};
	for (auto const& style : {
			 m_themeCache.normal,
			 m_themeCache.hover,
			 m_themeCache.pressed,
			 m_themeCache.hover_pressed,
			 m_themeCache.disabled })
	{
		if (style.IsValid())
		{
			for (int side = 0; side < 4; ++side)
			{
				margins[side] =
					std::max(margins[side], style->GetContentMargin(static_cast<ThemeStyleBox::Side>(side)));
			}
		}
	}

	Vector2 const start(margins[0], margins[1]);
	return AABB2(
		start,
		Vector2(std::max(start.x, GetSize().x - margins[2]), std::max(start.y, GetSize().y - margins[3])));
}

Vector2 Button::GetInternalMinimumSize() const
{
	Vector2 const textSize = GetContentSize();
	Vector2       marginSize;
	// Reserve the largest state margins so hovering never moves text or changes the hit rectangle.
	// e.g. A thicker pressed border can coexist with the normal state's content spacing.
	float margins[4] = {};
	for (auto const& style : {
			 m_themeCache.normal,
			 m_themeCache.hover,
			 m_themeCache.pressed,
			 m_themeCache.hover_pressed,
			 m_themeCache.disabled })
	{
		if (style.IsValid())
		{
			for (int side = 0; side < 4; ++side)
			{
				margins[side] =
					std::max(margins[side], style->GetContentMargin(static_cast<ThemeStyleBox::Side>(side)));
			}
		}
	}

	marginSize = Vector2(margins[0] + margins[2], margins[1] + margins[3]);
	Vector2 const minimum = textSize + marginSize;
	return Vector2(minimum.x, std::max(minimum.y, static_cast<float>(m_themeCache.minimum_height)));
}

void Button::SetText(String const& text)
{
	if (m_text != text)
	{
		m_text = text;
		PropagateMinimumSizeChanged();
		QueueRedraw();
	}
}

void Button::SetIcon(Ref<TextureResource> const& icon)
{
	if (m_icon != icon)
	{
		m_icon = icon;
		PropagateMinimumSizeChanged();
		QueueRedraw();
	}
}

void Button::SetClipText(bool enabled)
{
	if (m_clipText == enabled)
	{
		return;
	}

	m_clipText = enabled;
	PropagateMinimumSizeChanged();
	QueueRedraw();
}

void Button::SetTextOverrunBehavior(TextOverrunBehavior behavior)
{
	if (m_overrun == behavior)
	{
		return;
	}

	m_overrun = behavior;
	PropagateMinimumSizeChanged();
	QueueRedraw();
}

Vector2 Button::GetIconSize() const
{
	auto icon = GetIcon();
	if (!icon.IsValid() || icon->GetDimensions().x <= 0 || icon->GetDimensions().y <= 0)
	{
		return Vector2::Zero;
	}

	Vector2 size(icon->GetDimensions());
	float const limit = static_cast<float>(std::max(1, m_themeCache.icon_size));
	return size * (limit / std::max(size.x, size.y));
}

Vector2 Button::GetContentSize() const
{
	Vector2 textSize;
	if (!m_text.IsEmpty() && m_themeCache.font.IsValid() && m_themeCache.font->GetFontRID().IsValid()
		&& g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		TextLine const line = g_engine->m_textServer->GetTextLine(
			m_themeCache.font->GetFontRID(), m_text, static_cast<float>(m_themeCache.font_size));
		textSize = Vector2(line.m_width, std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent));
		if (m_clipText || m_overrun != TextOverrunBehavior::TrimNothing)
		{
			textSize.x = 1.f;
		}
	}

	Vector2 const iconSize = GetIconSize();
	float const separation = iconSize.x > 0.f && textSize.x > 0.f
		? static_cast<float>(m_themeCache.icon_separation) : 0.f;
	return Vector2(textSize.x + iconSize.x + separation, std::max(textSize.y, iconSize.y));
}

void Button::OnThemeChanged()
{
	QueueRedraw();
}

void Button::OnDrawModeChanged()
{
	QueueRedraw();
}

void Button::DrawContent(Color const& color)
{
	AABB2 const rect = GetTextRect();
	Vector2 const iconSize = GetIconSize();
	float const separation = iconSize.x > 0.f && !m_text.IsEmpty()
		? static_cast<float>(m_themeCache.icon_separation) : 0.f;
	float const available = std::max(0.f, rect.GetWidth() - iconSize.x - separation);
	TextServer* server = g_engine != nullptr ? g_engine->m_textServer : nullptr;
	RID const font = m_themeCache.font.IsValid() ? m_themeCache.font->GetFontRID() : RID::Invalid;
	TextLine line;
	if (server != nullptr && font.IsValid() && !m_text.IsEmpty())
	{
		line = server->GetTextLine(font, m_text, static_cast<float>(m_themeCache.font_size));
		line = server->TrimTextLine(font, line, static_cast<float>(m_themeCache.font_size), available, m_overrun);
	}

	float const height = std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent);
	float const displayedWidth = m_clipText ? std::min(available, line.m_width) : line.m_width;
	Vector2 const contentSize(iconSize.x + separation + displayedWidth, std::max(iconSize.y, height));
	Vector2 offset = rect.m_mins + (rect.GetDimensions() - contentSize) * 0.5f;
	if (m_alignment == HorizontalAlignment::Left)
	{
		offset.x = rect.m_mins.x;
	}
	else if (m_alignment == HorizontalAlignment::Right)
	{
		offset.x = rect.m_maxs.x - contentSize.x;
	}
	if (iconSize.x > 0.f)
	{
		Rect2 iconRect;
		iconRect.m_position = offset + Vector2(0.f, (contentSize.y - iconSize.y) * 0.5f);
		iconRect.m_size = iconSize;
		Rect2 source;
		source.m_position = Vector2::Zero;
		source.m_size = Vector2(GetIcon()->GetDimensions());
		DrawTextureRectRegion(GetIcon(), iconRect, source, color);
		offset.x += iconSize.x + separation;
	}

	if (line.m_glyphs.empty())
	{
		return;
	}

	float const textLeft = offset.x;
	offset.y += (contentSize.y - height) * 0.5f + line.m_config.m_ascent;
	for (TextLineGlyph const& glyph : line.m_glyphs)
	{
		GlyphTexture const* texture = server->GetOrCreateGlyphTexture(font, glyph.m_glyphIndex, m_themeCache.font_size);
		if (texture == nullptr || !texture->m_texture.IsValid())
		{
			continue;
		}

		GlyphImage const& bitmap = texture->m_bitmap;
		Rect2 target;
		target.m_position = offset + Vector2(glyph.m_posX + bitmap.m_offsetX, static_cast<float>(bitmap.m_offsetY));
		Vector2 const global = GetGlobalPosition();
		target.m_position = Vector2(std::round(target.m_position.x + global.x) - global.x,
			std::round(target.m_position.y + global.y) - global.y);
		target.m_size = Vector2(static_cast<float>(bitmap.m_width), static_cast<float>(bitmap.m_height));
		Rect2 source;
		source.m_position = Vector2::Zero;
		source.m_size = target.m_size;
		if (m_clipText)
		{
			Vector2 const start(std::max(textLeft, target.m_position.x), std::max(rect.m_mins.y, target.m_position.y));
			Vector2 const end(std::min(rect.m_maxs.x, target.m_position.x + target.m_size.x),
				std::min(rect.m_maxs.y, target.m_position.y + target.m_size.y));
			if (end.x <= start.x || end.y <= start.y)
			{
				continue;
			}

			source.m_position = start - target.m_position;
			source.m_size = end - start;
			target.m_position = start;
			target.m_size = source.m_size;
		}

		DrawTextureRectRegion(texture->m_texture, target, source, color);
	}
}

void Button::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	case Notification_Resized:
	case Notification_MinimumSizeChanged:
		QueueRedraw();
		break;
	case Notification_Draw:
	{
		Ref<ThemeStyleBox> style = m_themeCache.normal;
		Color color = m_themeCache.font_color;
		switch (GetDrawMode())
		{
		case DrawMode::Normal:
			break;
		case DrawMode::Hover:
			style = m_themeCache.hover;
			color = m_themeCache.font_hover_color;
			break;
		case DrawMode::Pressed:
			style = m_themeCache.pressed;
			color = m_themeCache.font_pressed_color;
			break;
		case DrawMode::HoverPressed:
			style = m_themeCache.hover_pressed;
			color = m_themeCache.font_hover_pressed_color;
			break;
		case DrawMode::Disabled:
			style = m_themeCache.disabled;
			color = m_themeCache.font_disabled_color;
			break;
		}

		DrawThemeStyleBox(style, GetLocalRect());
		DrawContent(color);
		if (HasFocus())
		{
			DrawThemeStyleBox(m_themeCache.focus, GetLocalRect());
		}
		break;
	}
	}
}

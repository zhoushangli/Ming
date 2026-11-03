#pragma once

#include "MingEngine/Scene/GUI/BaseButton.hpp"
#include "MingEngine/Core/String.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"

class Button : public BaseButton
{
	MCLASS(Button, BaseButton);

private:
	struct ThemeCache
	{
		Ref<ThemeStyleBox>   normal;
		Ref<ThemeStyleBox>   hover;
		Ref<ThemeStyleBox>   pressed;
		Ref<ThemeStyleBox>   hover_pressed;
		Ref<ThemeStyleBox>   disabled;
		Ref<ThemeStyleBox>   focus;
		Ref<FontResource>    font;
		Ref<TextureResource> icon;
		int                  icon_size       = 16;
		int                  icon_separation = 6;
		int                  font_size       = 16;
		int                  minimum_height  = 0;
		Color                font_color;
		Color                font_hover_color;
		Color                font_pressed_color;
		Color                font_hover_pressed_color;
		Color                font_disabled_color;
	};

public:
	Button();

	String const&        GetText() const { return m_text; }
	Ref<TextureResource> GetIcon() const { return m_icon.IsValid() ? m_icon : m_themeCache.icon; }
	Ref<FontResource>    GetFont() const { return m_themeCache.font; }
	int                  GetFontSize() const { return m_themeCache.font_size; }
	Vector2              GetInternalMinimumSize() const override;
	bool                 IsClipText() const { return m_clipText; }
	TextOverrunBehavior  GetTextOverrunBehavior() const { return m_overrun; }

	void SetText(String const& text);
	void SetIcon(Ref<TextureResource> const& icon);
	void SetFont(Ref<FontResource> const& font) { AddThemeFontOverride("font", font); }
	void SetFontSize(int fontSize) { AddThemeFontSizeOverride("font_size", fontSize); }
	void SetClipText(bool enabled);
	void SetTextOverrunBehavior(TextOverrunBehavior behavior);
	void SetHorizontalAlignment(HorizontalAlignment alignment)
	{
		m_alignment = alignment;
		QueueRedraw();
	}

protected:
	static void BindMethods();

	void OnThemeChanged() override;
	void OnDrawModeChanged() override;

	void OnNotification(int notification);

private:
	AABB2   GetTextRect() const;
	Vector2 GetContentSize() const;
	Vector2 GetIconSize() const;
	void    DrawContent(Color const& color);

private:
	String               m_text;
	HorizontalAlignment  m_alignment = HorizontalAlignment::Center;
	Ref<TextureResource> m_icon;
	bool                 m_clipText = false;
	TextOverrunBehavior  m_overrun  = TextOverrunBehavior::TrimNothing;
	// These are resolved values, populated by the explicit bindings in BindMethods.
	// e.g. A hover change selects hover; it does not modify the Theme resource.
	ThemeCache m_themeCache;
};

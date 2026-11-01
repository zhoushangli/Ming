#pragma once

#include "MingEngine/Core/String.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"
#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/GUI/GUIDefinitions.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

class Label : public Control
{
	MCLASS(Label, Control);

private:
	struct ThemeCache
	{
		Ref<FontResource> font;
		int              font_size  = 16;
		Color            font_color = Color::White;
		int              line_spacing = 0;
	};

public:
	Label() { SetMouseFilter(MouseFilter::Ignore); }

	Vector2 GetInternalMinimumSize() const override;

	String const&       GetText() const { return m_text; }
	Ref<FontResource>   GetFont() const { return m_themeCache.font; }
	int                 GetFontSize() const { return m_themeCache.font_size; }
	Color               GetFontColor() const { return m_themeCache.font_color; }
	HorizontalAlignment GetHorizontalAlignment() const { return m_horizontalAlignment; }
	VerticalAlignment   GetVerticalAlignment() const { return m_verticalAlignment; }
	bool IsClipText() const { return m_clipText; }
	TextOverrunBehavior GetTextOverrunBehavior() const { return m_overrun; }
	AutowrapMode GetAutowrapMode() const { return m_wrap; }

	void SetText(String const& text);
	void SetFont(Ref<FontResource> const& font);
	void SetFontSize(int fontSize);
	void SetFontColor(Color const& color);
	void SetHorizontalAlignment(HorizontalAlignment alignment);
	void SetVerticalAlignment(VerticalAlignment alignment);
	void SetClipText(bool enabled);
	void SetTextOverrunBehavior(TextOverrunBehavior behavior);
	void SetAutowrapMode(AutowrapMode mode);

protected:
	static void BindMethods();

	void OnThemeChanged() override;
	void OnNotification(int notification);

private:
	void EnsureTextLayout() const;
	void UpdateTextLayout();
	void DrawText();

private:
	HorizontalAlignment m_horizontalAlignment = HorizontalAlignment::Left;
	VerticalAlignment   m_verticalAlignment   = VerticalAlignment::Top;

	String m_text;
	bool m_clipText = false;
	TextOverrunBehavior m_overrun = TextOverrunBehavior::TrimNothing;
	AutowrapMode m_wrap = AutowrapMode::Off;
	bool m_updatingLayout = false;
	mutable bool m_layoutDirty = true;
	mutable RID m_layoutFont;
	mutable int m_layoutFontSize = 0;
	mutable int m_layoutLineSpacing = 0;
	mutable float m_layoutWidth = -1.f;
	mutable Vector2 m_minimum;
	mutable float m_textHeight = 0.f;
	mutable std::vector<TextLine> m_lines;
	// Cache fields are resolved values; all changes go through Theme or override setters.
	// e.g. SetFontSize(12) sets an override, then the registered binding writes font_size here.
	ThemeCache m_themeCache;
};

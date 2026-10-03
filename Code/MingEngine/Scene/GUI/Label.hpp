#pragma once

#include "MingEngine/Core/String.hpp"
#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"
#include "MingEngine/Scene/GUI/GUIDefinitions.hpp"

class Label : public Control
{
	MCLASS(Label, Control);

public:
	String const&     GetText() const { return m_text; }
	Ref<FontResource> GetFont() const { return m_font; }
	int               GetFontSize() const { return m_fontSize; }
	HorizontalAlignment GetHorizontalAlignment() const { return m_horizontalAlignment; }
	VerticalAlignment   GetVerticalAlignment() const { return m_verticalAlignment; }

	void SetText(String const& text);
	void SetFont(Ref<FontResource> const& font);
	void SetFontSize(int fontSize);
	void SetHorizontalAlignment(HorizontalAlignment alignment);
	void SetVerticalAlignment(VerticalAlignment alignment);

protected:
	static void BindMethods() {}

    void OnNotification(int notification);

private:
	HorizontalAlignment m_horizontalAlignment = HorizontalAlignment::Left;
	VerticalAlignment   m_verticalAlignment   = VerticalAlignment::Top;

	String            m_text;
	Ref<FontResource> m_font;
	int               m_fontSize = 16;
};
#pragma once

#include "MingEngine/Scene/GUI/BaseButton.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"

class Button : public BaseButton
{
	MCLASS(Button, BaseButton);

public:
	Button();

	String const&     GetText() const { return m_label->GetText(); }
	Ref<FontResource> GetFont() const { return m_label->GetFont(); }
	int               GetFontSize() const { return m_label->GetFontSize(); }
	Vector2           GetInternalMinimumSize() const override;

	void SetText(String const& text) { m_label->SetText(text); }
	void SetFont(Ref<FontResource> const& font) { m_label->SetFont(font); }
	void SetFontSize(int fontSize) { m_label->SetFontSize(fontSize); }

protected:
	static void BindMethods() {}

	void OnNotification(int notification);

private:
	Label*  m_label   = nullptr;
	Vector2 m_padding = Vector2(12.f, 8.f);
};

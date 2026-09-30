#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

class ColorRect : public Control
{
	MCLASS(ColorRect, Control);

public:
	void SetColor(Color const& color);

	Color GetColor() const { return m_color; }

protected:
	static void BindMethods() {}

	void OnNotification(int notification);

private:
	Color m_color = Color::White;
};
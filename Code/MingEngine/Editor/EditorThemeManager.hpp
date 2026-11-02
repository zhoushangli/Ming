#pragma once

#include "MingEngine/Scene/Resource/Theme.hpp"

class EditorThemeManager
{
public:
	static Ref<Theme> CreateTheme();
	static Ref<Theme> CreateScaledTheme(Ref<Theme> const& baseTheme, float scale = 1.25f);
};

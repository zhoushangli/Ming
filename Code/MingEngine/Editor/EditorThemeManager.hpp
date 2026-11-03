#pragma once

#include "MingEngine/Scene/Resource/Theme.hpp"
#include <filesystem>

class EditorThemeManager
{
public:
	static Ref<Theme>            CreateTheme();
	static std::filesystem::path GetSettingsDirectory();
	static float                 LoadScale();
	static bool                  SaveScale(float scale);
	static Ref<Theme>            CreateScaledTheme(Ref<Theme> const& baseTheme, float scale = 1.25f);
};

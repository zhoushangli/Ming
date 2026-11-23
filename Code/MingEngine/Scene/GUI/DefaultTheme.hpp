#pragma once

#include "MingEngine/Scene/Resource/Theme.hpp"

Ref<Theme> CreateDefaultTheme();

namespace ThemeRadii
{
inline constexpr float Square  = 0.f;
inline constexpr float Small   = 2.f;
inline constexpr float Control = 4.f;
inline constexpr float Panel   = 6.f;
inline constexpr float Popup   = 8.f;
}

void ApplyThemeCornerRadii(Ref<Theme> const& theme);

#pragma once

#include "ThirdParty/imgui/imgui.h"

namespace EditorUIStyle
{
inline ImVec4 FromRgb(int r, int g, int b, float a = 1.f) { return ImVec4(r / 255.f, g / 255.f, b / 255.f, a); }

inline ImVec4 ControlBackgroundColor() { return FromRgb(0x50, 0x59, 0x68); }

inline ImVec4 ControlBackgroundHoveredColor() { return FromRgb(0x50, 0x59, 0x68); }

inline ImVec4 ControlBackgroundActiveColor() { return FromRgb(0x56, 0x9E, 0xFF); }

inline float InspectorTabWidth() { return 42.f; }

inline float InspectorTabHeight() { return 34.f; }

inline ImVec2 InspectorTabButtonSize() { return ImVec2(InspectorTabWidth(), InspectorTabHeight()); }

inline ImVec2 InspectorTabIconSize()
{
	float const size = InspectorTabHeight() * 0.70f;
	return ImVec2(size, size);
}

inline float InspectorHeaderHeight() { return 36.f; }

inline ImVec2 InspectorHeaderIconSize()
{
	float const size = InspectorHeaderHeight() * 0.45f;
	return ImVec2(size, size);
}

inline float SceneTreeIconPixelSize() { return 16.f; }

inline ImVec2 SceneTreeIconSize() { return ImVec2(SceneTreeIconPixelSize(), SceneTreeIconPixelSize()); }

inline float FileTreeIconPixelSize() { return 16.f; }

inline ImVec2 FileTreeIconSize() { return ImVec2(FileTreeIconPixelSize(), FileTreeIconPixelSize()); }

inline float FileTreeTextSpacing() { return 4.f; }

inline float MainMenuBarHeight() { return 36.f; }

inline float MainMenuHorizontalPadding() { return 8.f; }

inline ImVec2 MainMenuFramePadding()
{
	float const padY = (MainMenuBarHeight() - ImGui::GetFontSize()) * 0.5f;
	return ImVec2(MainMenuHorizontalPadding(), padY > 0.f ? padY : 0.f);
}

inline ImVec2 MainMenuIconButtonSize()
{
	float const size = MainMenuBarHeight() - 2.f;
	return ImVec2(size, size);
}

inline ImVec2 MainMenuIconSize()
{
	float const size = MainMenuBarHeight() * 0.60f;
	return ImVec2(size, size);
}
} // namespace EditorUIStyle

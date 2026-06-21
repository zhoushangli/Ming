#pragma once

#include "ThirdParty/imgui/imgui.h"

#include <string>

namespace EditorUIWidgets
{
bool RenderIcon(std::string const& iconName, std::string const& fallbackIconName, ImVec2 size);

void RenderTreeRowContent(
	std::string const& iconName,
	std::string const& fallbackIconName,
	std::string const& label,
	ImVec2 rowMin,
	ImVec2 rowMax,
	ImVec2 iconSize);
} // namespace EditorUIWidgets

#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"

#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"

namespace EditorUIWidgets
{
bool RenderIcon(std::string const& iconName, std::string const& fallbackIconName, ImVec2 size)
{
	if (EditorIcons::RenderIcon(iconName, size))
	{
		return true;
	}

	if (!fallbackIconName.empty() && fallbackIconName != iconName)
	{
		return EditorIcons::RenderIcon(fallbackIconName, size);
	}

	return false;
}

void RenderTreeRowContent(
	std::string const& iconName,
	std::string const& fallbackIconName,
	std::string const& label,
	ImVec2 rowMin,
	ImVec2 rowMax,
	ImVec2 iconSize)
{
	ImGui::SameLine();

	float const iconY = rowMin.y + (rowMax.y - rowMin.y - iconSize.y) * 0.5f;
	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, iconY));
	RenderIcon(iconName, fallbackIconName, iconSize);

	ImGui::SameLine(0.f, EditorUIStyle::FileTreeTextSpacing());

	float const textY = rowMin.y + (rowMax.y - rowMin.y - ImGui::GetTextLineHeight()) * 0.5f;
	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, textY));
	ImGui::TextUnformatted(label.c_str());
}
} // namespace EditorUIWidgets

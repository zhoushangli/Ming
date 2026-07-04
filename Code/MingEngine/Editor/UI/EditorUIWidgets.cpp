#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"

#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"

#include "ThirdParty/imgui/misc/cpp/imgui_stdlib.h"

namespace EditorUIWidgets
{
namespace
{
bool DrawVectorComponent(
	char const*    label,
	float&         value,
	ImVec4 const&  labelColor,
	ImVec2 const&  barPos,
	int            index,
	float          groupWidth,
	float          groupGap,
	float          innerPad,
	float          labelValueGap)
{
	float const groupX = barPos.x + index * (groupWidth + groupGap);
	float const groupY = barPos.y;
	float const labelWidth = ImGui::CalcTextSize(label).x;

	ImGui::SetCursorScreenPos(ImVec2(groupX + innerPad, groupY));
	ImGui::PushStyleColor(ImGuiCol_Text, labelColor);
	ImGui::TextUnformatted(label);
	ImGui::PopStyleColor();

	float const dragX = groupX + innerPad + labelWidth + labelValueGap;
	float const dragW = groupWidth - innerPad * 2.f - labelWidth - labelValueGap;

	ImGui::SetCursorScreenPos(ImVec2(dragX, groupY));
	ImGui::SetNextItemWidth(dragW);

	char id[16];
	snprintf(id, sizeof(id), "##%s", label);
	return ImGui::DragFloat(id, &value, 0.1f, 0.0f, 0.0f, "%.2f");
}

bool PropertyTripleFloat(
	std::string const& label,
	char const*        id,
	float&             x,
	float&             y,
	float&             z)
{
	ImGui::TextUnformatted(label.c_str());

	ImGuiStyle const& style = ImGui::GetStyle();

	ImVec2 const barPos   = ImGui::GetCursorScreenPos();
	float const  barWidth = ImGui::GetContentRegionAvail().x;
	float const  barHeight = ImGui::GetFrameHeight();

	float constexpr groupGap      = 6.f;
	float constexpr innerPad      = 8.f;
	float constexpr labelValueGap = 8.f;

	float const groupWidth = (barWidth - groupGap * 2.f) / 3.f;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(
		barPos,
		ImVec2(barPos.x + barWidth, barPos.y + barHeight),
		IM_COL32(27, 33, 42, 255),
		style.FrameRounding);

	ImGui::PushID(id);
	ImGui::Dummy(ImVec2(barWidth, barHeight));

	ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(45, 52, 64, 180));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(55, 62, 76, 220));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, style.FramePadding.y));

	bool edited = false;
	edited |= DrawVectorComponent("x", x, ImVec4(0.9f, 0.25f, 0.25f, 1.f), barPos, 0, groupWidth, groupGap, innerPad, labelValueGap);
	edited |= DrawVectorComponent("y", y, ImVec4(0.35f, 0.8f, 0.35f, 1.f), barPos, 1, groupWidth, groupGap, innerPad, labelValueGap);
	edited |= DrawVectorComponent("z", z, ImVec4(0.35f, 0.5f, 0.9f, 1.f), barPos, 2, groupWidth, groupGap, innerPad, labelValueGap);

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);
	ImGui::PopID();

	return edited;
}
} // namespace

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

void BeginPropertyRow(std::string const& label)
{
	ImGui::Columns(2, nullptr, false);
	ImGui::TextUnformatted(label.c_str());
}

void NextPropertyColumn()
{
	ImGui::NextColumn();
}

void EndPropertyRow()
{
	ImGui::Columns(1);
}

bool PropertyBool(std::string const& label, char const* id, bool& value)
{
	BeginPropertyRow(label);
	NextPropertyColumn();
	bool const edited = ImGui::Checkbox(id, &value);
	EndPropertyRow();
	return edited;
}

bool PropertyInt(std::string const& label, char const* id, int& value)
{
	BeginPropertyRow(label);
	NextPropertyColumn();
	bool const edited = ImGui::DragInt(id, &value, 1.0f);
	EndPropertyRow();
	return edited;
}

bool PropertyFloat(std::string const& label, char const* id, float& value)
{
	BeginPropertyRow(label);
	NextPropertyColumn();
	bool const edited = ImGui::DragFloat(id, &value, 0.1f, 0.0f, 0.0f, "%.2f");
	EndPropertyRow();
	return edited;
}

bool PropertyString(std::string const& label, char const* id, std::string& value)
{
	BeginPropertyRow(label);
	NextPropertyColumn();
	ImGui::InputText(id, &value);
	bool const edited = ImGui::IsItemDeactivatedAfterEdit();
	EndPropertyRow();
	return edited;
}

bool PropertyVec3(std::string const& label, char const* id, Vec3& value)
{
	return PropertyTripleFloat(label, id, value.x, value.y, value.z);
}

bool PropertyEuler(std::string const& label, char const* id, EulerAngles& value)
{
	return PropertyTripleFloat(label, id, value.m_rollDegrees, value.m_pitchDegrees, value.m_yawDegrees);
}
} // namespace EditorUIWidgets

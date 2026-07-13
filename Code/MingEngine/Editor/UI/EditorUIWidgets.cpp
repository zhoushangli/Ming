#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"

#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"

#include "ThirdParty/imgui/misc/cpp/imgui_stdlib.h"

#include <cmath>

namespace EditorUIWidgets
{
namespace
{
bool IsTripleFloatDefault(
	float x,
	float y,
	float z,
	float defaultX,
	float defaultY,
	float defaultZ)
{
	constexpr float epsilon = 1e-5f;
	return std::abs(x - defaultX) <= epsilon
		&& std::abs(y - defaultY) <= epsilon
		&& std::abs(z - defaultZ) <= epsilon;
}

bool DrawPropertyResetButton(char const* id, float y)
{
	float const       buttonSize = ImGui::GetTextLineHeight();
	ImVec2 const      buttonPos(
		ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - buttonSize,
		y);

	ImGui::SetCursorScreenPos(buttonPos);
	std::string const buttonId = std::string(id) + "_reset";
	bool const clicked = ImGui::InvisibleButton(buttonId.c_str(), ImVec2(buttonSize, buttonSize));

	ImTextureID const textureId = EditorIcons::GetIconId("ReloadSmall", "Reload");
	float const       iconInset = 2.f;
	EditorIcons::AddImage(
		ImGui::GetWindowDrawList(),
		textureId,
		ImVec2(buttonPos.x + iconInset, buttonPos.y + iconInset),
		ImVec2(buttonPos.x + buttonSize - iconInset, buttonPos.y + buttonSize - iconInset));

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset");
	}

	return clicked;
}

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
	float&             z,
	float              defaultX,
	float              defaultY,
	float              defaultZ)
{
	bool reset = false;
	ImVec2 const labelPos = ImGui::GetCursorScreenPos();
	ImGui::TextUnformatted(label.c_str());
	if (!IsTripleFloatDefault(x, y, z, defaultX, defaultY, defaultZ))
	{
		ImGui::PushID(id);
		reset = DrawPropertyResetButton("property", labelPos.y);
		ImGui::PopID();
		if (reset)
		{
			x = defaultX;
			y = defaultY;
			z = defaultZ;
		}
	}
	ImGui::SetCursorScreenPos(ImVec2(labelPos.x, labelPos.y + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y));

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
		IM_COL32(0x21, 0x25, 0x2B, 0xFF),
		style.FrameRounding);

	ImGui::PushID(id);
	ImGui::Dummy(ImVec2(barWidth, barHeight));

	ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0x50, 0x59, 0x68, 0xCC));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0x50, 0x59, 0x68, 0xE0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, style.FramePadding.y));

	bool edited = reset;
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

	ImGuiStyle const& style       = ImGui::GetStyle();
	ImVec2 const      framePos    = ImGui::GetCursorScreenPos();
	float const       frameWidth  = ImGui::GetContentRegionAvail().x;
	float const       frameHeight = ImGui::GetFrameHeight();

	std::string const buttonId = std::string(id) + "_bool_field";
	ImGui::InvisibleButton(buttonId.c_str(), ImVec2(frameWidth, frameHeight));
	bool const hovered = ImGui::IsItemHovered();
	bool const active  = ImGui::IsItemActive();
	bool const edited  = ImGui::IsItemClicked(ImGuiMouseButton_Left);
	if (edited)
	{
		value = !value;
	}

	ImVec4 const bgColor =
		active ? ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive)
			   : (hovered ? ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered)
						  : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 const frameMax(framePos.x + frameWidth, framePos.y + frameHeight);
	drawList->AddRectFilled(framePos, frameMax, ImGui::GetColorU32(bgColor), style.FrameRounding);

	float const boxSize = ImGui::GetFontSize() * 0.72f;
	ImVec2 const boxMin(
		framePos.x + style.FramePadding.x,
		framePos.y + (frameHeight - boxSize) * 0.5f);
	ImVec2 const boxMax(boxMin.x + boxSize, boxMin.y + boxSize);

	ImU32 const boxColor =
		value ? ImGui::GetColorU32(ImGuiCol_CheckMark) : ImGui::GetColorU32(ImGuiCol_FrameBgActive);
	drawList->AddRectFilled(boxMin, boxMax, boxColor, style.FrameRounding * 0.45f);

	if (value)
	{
		float const thickness = 2.f;
		ImU32 const markColor = ImGui::GetColorU32(ImGuiCol_Text);
		ImVec2 const a(boxMin.x + boxSize * 0.22f, boxMin.y + boxSize * 0.52f);
		ImVec2 const b(boxMin.x + boxSize * 0.42f, boxMin.y + boxSize * 0.72f);
		ImVec2 const c(boxMin.x + boxSize * 0.78f, boxMin.y + boxSize * 0.28f);
		drawList->AddLine(a, b, markColor, thickness);
		drawList->AddLine(b, c, markColor, thickness);
	}

	char const* valueText = value ? "On" : "Off";
	ImVec2 const textSize = ImGui::CalcTextSize(valueText);
	ImVec2 const textPos(
		boxMax.x + style.ItemInnerSpacing.x,
		framePos.y + (frameHeight - textSize.y) * 0.5f);
	drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), valueText);

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

bool PropertyVec3(std::string const& label, char const* id, Vec3& value, Vec3 const& defaultValue)
{
	return PropertyTripleFloat(label, id, value.x, value.y, value.z, defaultValue.x, defaultValue.y, defaultValue.z);
}

bool PropertyEuler(std::string const& label, char const* id, EulerAngles& value)
{
	return PropertyTripleFloat(label, id, value.m_rollDegrees, value.m_pitchDegrees, value.m_yawDegrees, 0.f, 0.f, 0.f);
}
} // namespace EditorUIWidgets

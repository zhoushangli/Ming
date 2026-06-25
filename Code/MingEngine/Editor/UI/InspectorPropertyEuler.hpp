#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "ThirdParty/imgui/imgui.h"

class InspectorPropertyEuler final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(Variant const& value) override
	{
		EulerAngles v = value.As<EulerAngles>();

		ImGui::TextUnformatted(GetDisplayName().c_str());

		ImGuiStyle const& style = ImGui::GetStyle();

		ImVec2 const barPos   = ImGui::GetCursorScreenPos();
		float const barWidth  = ImGui::GetContentRegionAvail().x;
		float const barHeight = ImGui::GetFrameHeight();

		float constexpr groupGap      = 6.f;
		float constexpr innerPad      = 8.f;
		float constexpr labelValueGap = 8.f;

		float const groupWidth = (barWidth - groupGap * 2.f) / 3.f;

		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(
			barPos,
			ImVec2(barPos.x + barWidth, barPos.y + barHeight),
			IM_COL32(27, 33, 42, 255),
			style.FrameRounding);

		ImGui::PushID(m_labelId.c_str());

		ImGui::Dummy(ImVec2(barWidth, barHeight));

		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(45, 52, 64, 180));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(55, 62, 76, 220));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, style.FramePadding.y));

		bool edited = false;

		edited |= DrawComponent(
			"x",
			v.m_rollDegrees,
			ImVec4(0.9f, 0.25f, 0.25f, 1.f),
			barPos,
			0,
			groupWidth,
			groupGap,
			innerPad,
			labelValueGap,
			barHeight);

		edited |= DrawComponent(
			"y",
			v.m_pitchDegrees,
			ImVec4(0.35f, 0.8f, 0.35f, 1.f),
			barPos,
			1,
			groupWidth,
			groupGap,
			innerPad,
			labelValueGap,
			barHeight);

		edited |= DrawComponent(
			"z",
			v.m_yawDegrees,
			ImVec4(0.35f, 0.5f, 0.9f, 1.f),
			barPos,
			2,
			groupWidth,
			groupGap,
			innerPad,
			labelValueGap,
			barHeight);

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
		ImGui::PopID();

		if (edited)
		{
			EmitValueChanged(Variant(v));
		}
	}

private:
	static bool DrawComponent(
		char const* label,
		float& value,
		ImVec4 const& labelColor,
		ImVec2 const& barPos,
		int index,
		float groupWidth,
		float groupGap,
		float innerPad,
		float labelValueGap,
		float barHeight)
	{
		barHeight;
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
};


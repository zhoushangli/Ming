#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "ThirdParty/imgui/imgui.h"

class InspectorPropertyEuler final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void Render() override
	{
		EulerAngles e = m_value.As<EulerAngles>();

		// Row 1) Label
		ImGui::TextUnformatted(GetDisplayName().c_str());

		// Row 2) Draw one continuous dark background, then overlay controls
		ImVec2 const barPos   = ImGui::GetCursorScreenPos();
		float const barWidth  = ImGui::GetContentRegionAvail().x;
		float const barHeight = ImGui::GetFrameHeight();
		float const third     = barWidth / 3.0f;

		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(
			barPos,
			ImVec2(barPos.x + barWidth, barPos.y + barHeight),
			IM_COL32(27, 33, 42, 255),
			ImGui::GetStyle().FrameRounding);

		// Transparent FrameBg — the drawn rect is the background
		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
		ImGui::PushID(m_labelId.c_str());

		float const labelXw = ImGui::CalcTextSize("x").x;
		float const labelYw = ImGui::CalcTextSize("y").x;
		float const labelZw = ImGui::CalcTextSize("z").x;

		bool edited = false;

		// Yaw / X (red label, white number)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
		ImGui::TextUnformatted("x");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		ImGui::SetNextItemWidth(third - labelXw);
		edited = ImGui::DragFloat("##yaw", &e.m_yawDegrees, 0.1f, 0.0f, 0.0f, "%.1f");
		ImGui::SameLine(0, 0);

		// Pitch / Y (green label, white number)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.8f, 0.35f, 1.0f));
		ImGui::TextUnformatted("y");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		ImGui::SetNextItemWidth(third - labelYw);
		edited |= ImGui::DragFloat("##pitch", &e.m_pitchDegrees, 0.1f, 0.0f, 0.0f, "%.1f");
		ImGui::SameLine(0, 0);

		// Roll / Z (blue label, white number)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.5f, 0.9f, 1.0f));
		ImGui::TextUnformatted("z");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		ImGui::SetNextItemWidth(third - labelZw);
		edited |= ImGui::DragFloat("##roll", &e.m_rollDegrees, 0.1f, 0.0f, 0.0f, "%.1f");

		ImGui::PopID();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		if (edited)
		{
			m_value  = Variant(e);
			m_edited = true;
		}
	}
};

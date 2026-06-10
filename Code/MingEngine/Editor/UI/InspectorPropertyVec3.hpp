#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"
#include "ThirdParty/imgui/imgui.h"

#include <cmath>

class InspectorPropertyVec3 final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void Render() override
	{
		Vec3 v = m_value.As<Vec3>();

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

		// X (red label, white number)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
		ImGui::TextUnformatted("x");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		ImGui::SetNextItemWidth(third - labelXw);
		edited = ImGui::DragFloat("##x", &v.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::SameLine(0, 0);

		// Y (green label, white number)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.8f, 0.35f, 1.0f));
		ImGui::TextUnformatted("y");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		ImGui::SetNextItemWidth(third - labelYw);
		edited |= ImGui::DragFloat("##y", &v.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::SameLine(0, 0);

		// Z (blue label, white number)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.5f, 0.9f, 1.0f));
		ImGui::TextUnformatted("z");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 0);
		ImGui::SetNextItemWidth(third - labelZw);
		edited |= ImGui::DragFloat("##z", &v.z, 0.1f, 0.0f, 0.0f, "%.2f");

		ImGui::PopID();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		// Clamp scale components
		if (m_info.m_name == "scale")
		{
			if (std::abs(v.x) < 1e-5f)
				v.x = 1e-5f;
			if (std::abs(v.y) < 1e-5f)
				v.y = 1e-5f;
			if (std::abs(v.z) < 1e-5f)
				v.z = 1e-5f;
		}

		if (edited)
		{
			m_value  = Variant(v);
			m_edited = true;
		}
	}
};

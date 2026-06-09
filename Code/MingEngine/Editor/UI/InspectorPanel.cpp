#include "MingEngine/Editor/UI/InspectorPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

InspectorPanel::InspectorPanel()
	: EditorPanel("Inspector")
{
}

void InspectorPanel::OnRender([[maybe_unused]] EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());
	ImGui::TextUnformatted("Selected: Player");
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Position", m_position, 0.1f, 0.0f, 0.f, "%.1f");
		ImGui::DragFloat3("Rotation", m_rotation, 0.1f, 0.0f, 0.f, "%.1f");
		ImGui::DragFloat3("Scale", m_scale, 0.1f, 0.0f, 0.f, "%.1f");
	}

	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Visible", &m_visible);
		ImGui::TextUnformatted("Material: Default");
		ImGui::TextUnformatted("Mesh: Cube");
	}
	ImGui::End();
}

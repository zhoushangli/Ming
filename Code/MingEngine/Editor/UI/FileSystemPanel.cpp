#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

FileSystemPanel::FileSystemPanel()
	: m_panel("FileSystem")
{
}

void FileSystemPanel::Render()
{
	if (!m_panel.IsOpen())
	{
		return;
	}

	ImGui::Begin(m_panel.GetTitle(), m_panel.GetOpenState());
	ImGui::TextUnformatted("res://");
	ImGui::Separator();
	ImGui::InputTextWithHint("##FilterFiles", "Filter Files", m_filter, sizeof(m_filter));

	if (ImGui::TreeNodeEx("Data", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TreeNodeEx("Images", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		if (ImGui::TreeNodeEx("Models", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TreeNodeEx("Cube.fbx", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreeNodeEx("Character.fbx", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreePop();
		}
		ImGui::TreeNodeEx("Shaders", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreeNodeEx("Audio", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreePop();
	}
	ImGui::End();
}

EditorPanel& FileSystemPanel::GetPanel() { return m_panel; }

EditorPanel const& FileSystemPanel::GetPanel() const { return m_panel; }

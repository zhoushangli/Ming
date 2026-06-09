#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

FileSystemPanel::FileSystemPanel()
	: EditorPanel("FileSystem")
{
}

void FileSystemPanel::OnRender([[maybe_unused]] EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());
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

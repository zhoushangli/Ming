#include "MingEngine/Editor/UI/OutputPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

OutputPanel::OutputPanel()
	: EditorPanel("Output")
{
}

void OutputPanel::OnRender([[maybe_unused]] EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());
	if (ImGui::BeginTabBar("OutputTabs"))
	{
		if (ImGui::BeginTabItem("Output"))
		{
			ImGui::TextUnformatted("[Info] Editor started");
			ImGui::TextUnformatted("[Info] Loaded scene: Untitled");
			ImGui::TextUnformatted("[Warning] Viewport is a placeholder in this phase");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Console"))
		{
			ImGui::TextUnformatted("> Console input placeholder");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Profiler"))
		{
			ImGui::TextUnformatted("Frame: -- ms");
			ImGui::TextUnformatted("Draw Calls: --");
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

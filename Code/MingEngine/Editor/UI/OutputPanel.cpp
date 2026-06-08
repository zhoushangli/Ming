#include "MingEngine/Editor/UI/OutputPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

OutputPanel::OutputPanel()
	: m_panel("Output")
{
}

void OutputPanel::Render()
{
	if (!m_panel.IsOpen())
	{
		return;
	}

	ImGui::Begin(m_panel.GetTitle(), m_panel.GetOpenState());
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

EditorPanel& OutputPanel::GetPanel() { return m_panel; }

EditorPanel const& OutputPanel::GetPanel() const { return m_panel; }

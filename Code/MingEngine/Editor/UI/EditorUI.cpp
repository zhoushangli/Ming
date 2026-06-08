#include "MingEngine/Editor/UI/EditorUI.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"

#include "ThirdParty/imgui/imgui.h"

void EditorUI::Render(EditorUIContext& context)
{
	RenderMainMenuBar();
	RenderDockSpace();

	m_scenePanel.Render(context, m_createNodePanel);
	m_fileSystemPanel.Render();
	m_viewportPanel.Render(context);
	m_inspectorPanel.Render();
	m_outputPanel.Render();
	m_createNodePanel.Render(context);
}

void EditorUI::RenderMainMenuBar()
{
	if (!ImGui::BeginMainMenuBar())
	{
		return;
	}

	if (ImGui::BeginMenu("Scene"))
	{
		ImGui::MenuItem("New Scene");
		ImGui::MenuItem("Open Scene...");
		ImGui::Separator();
		ImGui::MenuItem("Save Scene");
		ImGui::MenuItem("Save Scene As...");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Project"))
	{
		ImGui::MenuItem("Project Settings...");
		ImGui::MenuItem("Reload Project");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Debug"))
	{
		ImGui::MenuItem("Start");
		ImGui::MenuItem("Pause");
		ImGui::MenuItem("Stop");
		ImGui::MenuItem("Step Frame");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Editor"))
	{
		ImGui::MenuItem("Editor Settings...");
		ImGui::MenuItem("Reset Layout");
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Scene", nullptr, m_scenePanel.GetPanel().GetOpenState());
		ImGui::MenuItem("FileSystem", nullptr, m_fileSystemPanel.GetPanel().GetOpenState());
		ImGui::MenuItem("Viewport", nullptr, m_viewportPanel.GetPanel().GetOpenState());
		ImGui::MenuItem("Inspector", nullptr, m_inspectorPanel.GetPanel().GetOpenState());
		ImGui::MenuItem("Output", nullptr, m_outputPanel.GetPanel().GetOpenState());
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Help"))
	{
		ImGui::MenuItem("About Project QingChen");
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void EditorUI::RenderDockSpace()
{
	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	windowFlags |= ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::Begin("QingChen Editor DockSpace", nullptr, windowFlags);
	ImGui::PopStyleVar(2);

	ImGuiID dockspaceId = ImGui::GetID("QingChenEditorDockSpace");
	ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);
	ImGui::End();
}

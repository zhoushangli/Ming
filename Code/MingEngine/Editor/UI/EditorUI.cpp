#include "MingEngine/Editor/UI/EditorUI.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"

#include "EditorUI.hpp"
#include "ThirdParty/imgui/imgui.h"

namespace
{
void RenderPanelMenuItem(EditorPanel& panel)
{
	if (!ImGui::MenuItem(panel.GetTitle(), nullptr, panel.IsOpen()))
	{
		return;
	}

	if (panel.IsOpen())
	{
		panel.Close();
	}
	else
	{
		panel.Open();
	}
}
} // namespace

void EditorUI::Render(EditorUIContext& context)
{
	context.m_editorUI = this;

	RenderMainMenuBar();
	RenderDockSpace();

	m_scenePanel.Render(context);
	m_fileSystemPanel.Render(context);
	m_viewportPanel.Render(context);
	m_inspectorPanel.Render(context);
	m_outputPanel.Render(context);
	m_createNodePanel.Render(context);

	RenderWarningPopup();
}

void EditorUI::Warning(std::string const& title, std::string const& message)
{
	m_warningData.m_title   = title;
	m_warningData.m_message = message;
	m_showWarningPopup      = true;
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
		RenderPanelMenuItem(m_scenePanel);
		RenderPanelMenuItem(m_fileSystemPanel);
		RenderPanelMenuItem(m_viewportPanel);
		RenderPanelMenuItem(m_inspectorPanel);
		RenderPanelMenuItem(m_outputPanel);
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

void EditorUI::RenderWarningPopup()
{
	constexpr char const* popupId     = "WarningPopup";
	constexpr float       popupWidth  = 420.f;
	constexpr float       buttonWidth = 120.f;

	if (m_showWarningPopup)
	{
		ImGui::OpenPopup(popupId);
		m_showWarningPopup = false;
	}

	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImVec2 const popupCenter(
		viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
		viewport->WorkPos.y + viewport->WorkSize.y * 0.5f);
	ImGui::SetNextWindowPos(popupCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(popupWidth, 0.f), ImGuiCond_Appearing);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.f, 18.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.f, 12.f));

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::BeginPopupModal(popupId, nullptr, windowFlags))
	{
		ImGuiStyle const& style = ImGui::GetStyle();

		ImGui::PushFont(nullptr, style.FontSizeBase * 1.15f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.72f, 0.24f, 1.f));
		ImGui::TextWrapped("WARNING: %s", m_warningData.m_title.c_str());
		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::Separator();

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.85f, 0.9f, 1.f));
		ImGui::TextWrapped("%s", m_warningData.m_message.c_str());
		ImGui::PopStyleColor();

		ImGui::Spacing();

		float const buttonX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth;
		ImGui::SetCursorPosX(buttonX);

		bool const shouldClose = ImGui::Button("OK", ImVec2(buttonWidth, 0.f))
								 || ImGui::IsKeyPressed(ImGuiKey_Enter, false)
								 || ImGui::IsKeyPressed(ImGuiKey_Escape, false);
		if (shouldClose)
		{
			ImGui::CloseCurrentPopup();
			m_warningData = {};
		}

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
}

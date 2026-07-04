#include "MingEngine/Editor/UI/EditorUI.hpp"

#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"

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

bool RenderMainMenuIconButton(char const* id, char const* iconName, char const* tooltip)
{
	ImVec2 const iconSize = EditorUIStyle::MainMenuIconSize();
	ImVec2 const buttonSize = EditorUIStyle::MainMenuIconButtonSize();

	ImTextureID const textureId = EditorIcons::GetIconId(iconName);
	ImGui::PushStyleColor(ImGuiCol_Button, EditorUIStyle::ControlBackgroundColor());
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorUIStyle::ControlBackgroundHoveredColor());
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, EditorUIStyle::ControlBackgroundActiveColor());
	bool const clicked = ImGui::Button(id, buttonSize);
	ImGui::PopStyleColor(3);

	if (textureId != ImTextureID{})
	{
		ImVec2 const itemMin = ImGui::GetItemRectMin();
		ImVec2 const itemMax = ImGui::GetItemRectMax();
		ImVec2 const itemCenter((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f);
		ImVec2 const iconMin(itemCenter.x - iconSize.x * 0.5f, itemCenter.y - iconSize.y * 0.5f);
		EditorIcons::AddImage(
			ImGui::GetWindowDrawList(),
			textureId,
			iconMin,
			ImVec2(iconMin.x + iconSize.x, iconMin.y + iconSize.y));
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", tooltip);
	}

	return clicked;
}
} // namespace

void EditorUI::Render(EditorUIContext& context)
{
	context.m_editorUI = this;

	RenderMainMenuBar();
	RenderDockSpace();

	BeginResourceDragDropFrame();

	m_scenePanel.Render(context);
	m_fileSystemPanel.Render(context);
	m_viewportPanel.Render(context);
	m_inspectorPanel.Render(context);
	m_outputPanel.Render(context);
	m_createNodePanel.Render(context);

	ApplyResourceDragDropCursor();
	RenderWarningPopup();
}

void EditorUI::Warning(std::string const& title, std::string const& message)
{
	m_warningData.m_title   = title;
	m_warningData.m_message = message;
	m_showWarningPopup      = true;
}

void EditorUI::SetResourceDropAllowed(bool allowed)
{
	m_resourceDropAllowed = m_resourceDropAllowed || allowed;
}

void EditorUI::BeginResourceDragDropFrame()
{
	m_resourceDropAllowed = false;
}

void EditorUI::ApplyResourceDragDropCursor()
{
	ImGuiPayload const* payload = ImGui::GetDragDropPayload();
	if (payload == nullptr || !payload->IsDataType("FILESYSTEM_RESOURCE"))
	{
		return;
	}

	ImGui::SetMouseCursor(m_resourceDropAllowed ? ImGuiMouseCursor_Arrow : ImGuiMouseCursor_NotAllowed);
}

void EditorUI::RenderMainMenuBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, EditorUIStyle::MainMenuFramePadding());
	if (!ImGui::BeginMainMenuBar())
	{
		ImGui::PopStyleVar();
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

	ImGuiStyle const& style = ImGui::GetStyle();
	ImVec2 const      buttonSize = EditorUIStyle::MainMenuIconButtonSize();
	float const       toolbarWidth = buttonSize.x * 2.f + style.ItemSpacing.x;
	float const       toolbarX = ImGui::GetWindowWidth() - toolbarWidth - style.FramePadding.x;
	if (toolbarX > ImGui::GetCursorPosX())
	{
		ImGui::SetCursorPosX(toolbarX);
	}

	if (RenderMainMenuIconButton("##EditorPlay", "Play", "Start"))
	{
		// Handle play button click
	}

	ImGui::SameLine();

	if (RenderMainMenuIconButton("##EditorStop", "Stop", "Stop"))
	{
		// Handle stop button click
	}

	ImGui::EndMainMenuBar();
	ImGui::PopStyleVar();
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

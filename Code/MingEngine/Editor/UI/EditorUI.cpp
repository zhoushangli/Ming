#include "MingEngine/Editor/UI/EditorUI.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
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
	ImVec2 const iconSize   = EditorUIStyle::MainMenuIconSize();
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
	context.m_editorUI       = this;
	EditorDragDrop& dragDrop = EditorNode::Get()->m_dragDrop;

	dragDrop.BeginFrame();

	RenderMainMenuBar();
	RenderDockSpace();

	m_scenePanel.Render(context);
	m_importPanel.Render(context);
	m_fileSystemPanel.Render(context);
	m_viewportPanel.Render(context);
	m_inspectorPanel.Render(context);
	m_outputPanel.Render(context);
	m_projectSettingsPopup.Render(context);
	m_warningPopup.Render(context);

	ApplyDragDropCursor();
	
	ImGuiPayload const* payload = ImGui::GetDragDropPayload();
	bool isDragging = payload != nullptr && payload->IsDataType(EditorDragDrop::PayloadType);

	dragDrop.EndFrame(isDragging);
}

void EditorUI::Warning(std::string const& title, std::string const& message) { m_warningPopup.Open(title, message); }

void EditorUI::SetViewportRect(Vec2 origin, Vec2 dims)
{
	m_viewportOrigin = origin;
	m_viewportDims   = dims;
}

Vec2 EditorUI::GetViewportOrigin() const { return m_viewportOrigin; }
Vec2 EditorUI::GetViewportDimensions() const { return m_viewportDims; }
Vec2 EditorUI::ToViewportPos(Vec2 windowPos) const { return windowPos - m_viewportOrigin; }

void EditorUI::ApplyDragDropCursor()
{
	ImGuiPayload const* payload = ImGui::GetDragDropPayload();
	if (payload == nullptr || !payload->IsDataType(EditorDragDrop::PayloadType))
	{
		return;
	}

	ImGui::SetMouseCursor(
		EditorNode::Get()->m_dragDrop.IsDropAllowed() ? ImGuiMouseCursor_Arrow : ImGuiMouseCursor_NotAllowed);
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
		if (ImGui::MenuItem("Project Settings..."))
		{
			m_projectSettingsPopup.Open();
		}
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
		RenderPanelMenuItem(m_importPanel);
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

	ImGuiStyle const& style        = ImGui::GetStyle();
	ImVec2 const      buttonSize   = EditorUIStyle::MainMenuIconButtonSize();
	float const       toolbarWidth = buttonSize.x * 2.f + style.ItemSpacing.x;
	float const       toolbarX     = ImGui::GetWindowWidth() - toolbarWidth - style.FramePadding.x;
	if (toolbarX > ImGui::GetCursorPosX())
	{
		ImGui::SetCursorPosX(toolbarX);
	}

	if (RenderMainMenuIconButton("##EditorPlay", "Play", "Start"))
	{
		EditorNode::Get()->PlayScene();
	}

	ImGui::SameLine();

	if (RenderMainMenuIconButton("##EditorStop", "Stop", "Stop"))
	{
		EditorNode::Get()->StopScene();
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

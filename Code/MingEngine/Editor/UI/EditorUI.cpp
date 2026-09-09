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

	ImGuiPayload const* payload    = ImGui::GetDragDropPayload();
	bool                isDragging = payload != nullptr && payload->IsDataType(EditorDragDrop::PayloadType);

	dragDrop.EndFrame(isDragging);
}

void EditorUI::Warning(std::string const& title, std::string const& message) { m_warningPopup.Open(title, message); }

void EditorUI::SetViewportRect(Vector2 origin, Vector2 dims)
{
	m_viewportOrigin = origin;
	m_viewportDims   = dims;
}

Vector2 EditorUI::GetViewportOrigin() const { return m_viewportOrigin; }
Vector2 EditorUI::GetViewportDimensions() const { return m_viewportDims; }
Vector2 EditorUI::ToViewportPos(Vector2 windowPos) const { return windowPos - m_viewportOrigin; }

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

	// 1) Align the icon buttons to the right side of the menu bar
	int const    buttonCount    = 3;
	ImVec2 const buttonSize     = EditorUIStyle::MainMenuIconButtonSize();
	ImVec2 const iconSize       = EditorUIStyle::MainMenuIconSize();
	float const  spacing        = ImGui::GetStyle().ItemSpacing.x;
	float const  rightPadding   = EditorUIStyle::MainMenuHorizontalPadding();
	float const  totalWidth     = buttonSize.x * (float)buttonCount + spacing * (float)(buttonCount - 1);
	float const  remainingWidth = ImGui::GetContentRegionAvail().x;
	if (remainingWidth > totalWidth + rightPadding)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + remainingWidth - totalWidth - rightPadding);
	}
	ImGui::SetCursorPosY((ImGui::GetWindowHeight() - buttonSize.y) * 0.5f);

	// 2) Draw the Play and Stop buttons without attaching actions
	ImGui::PushStyleVar(
		ImGuiStyleVar_FramePadding,
		ImVec2((buttonSize.x - iconSize.x) * 0.5f, (buttonSize.y - iconSize.y) * 0.5f));

	if (ImGui::ImageButton(
			"##MainMenuBuildCSharp",
			EditorIcons::GetIconId("BuildCSharp"),
			iconSize,
			ImVec2(0.f, 1.f),
			ImVec2(1.f, 0.f)))
	{
		if (g_engine == nullptr || g_engine->m_scriptSystem == nullptr)
		{
			DebuggerPrintf("Script system is unavailable.\n");
		}
		else
		{
			g_engine->m_scriptSystem->EnsureProjectSolution();
		}
	}

	ImGui::SameLine(0.f, spacing);

	if (ImGui::ImageButton(
			"##MainMenuPlay",
			EditorIcons::GetIconId("Play"),
			iconSize,
			ImVec2(0.f, 1.f),
			ImVec2(1.f, 0.f)))
	{
		DebuggerPrintf("Play button clicked\n");
	}

	ImGui::SameLine(0.f, spacing);

	if (ImGui::ImageButton(
			"##MainMenuStop",
			EditorIcons::GetIconId("Stop"),
			iconSize,
			ImVec2(0.f, 1.f),
			ImVec2(1.f, 0.f)))
	{
	}

	ImGui::PopStyleVar();

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

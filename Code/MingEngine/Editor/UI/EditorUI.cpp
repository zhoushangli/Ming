#include "MingEngine/Editor/UI/EditorUI.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"

#include "ThirdParty/imgui/imgui.h"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"

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

void EditorUI::RenderLegacy(EditorUIContext& context)
{
	m_inspectorPanel.GetHost().BeginFrame();
	m_projectSettingsPopup.GetHost().BeginFrame();
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
	m_inspectorPanel.GetHost().EndFrame();
	m_projectSettingsPopup.GetHost().EndFrame();

	ApplyDragDropCursor();

	ImGuiPayload const* payload    = ImGui::GetDragDropPayload();
	bool                isDragging = payload != nullptr && payload->IsDataType(EditorDragDrop::PayloadType);

	dragDrop.EndFrame(isDragging);
}

bool EditorUI::HandleInput(InputEvent const& event)
{
	if (g_engine->m_imguiSystem == nullptr)
	{
		if (m_owner == nullptr)
		{
			return false;
		}

		if (event.m_type == InputEventType::WindowFocus && !event.m_focused)
		{
			m_navigating = false;
		}

		UpdateNativeDocking();
		m_owner->GetSceneTree()->FlushContainerUpdate();
		Viewport* root = static_cast<Viewport*>(m_owner->GetSceneTree()->GetRoot());
		root->ProcessInputEvent(event);
		UpdateNativeDocking();
		root->UpdateGuiDrag(root->GetGuiDragPosition());
		return true;
	}

	if (event.m_type == InputEventType::WindowFocus || event.m_type == InputEventType::CursorEnter)
	{
		m_inspectorPanel.GetHost().HandleInput(event);
		m_projectSettingsPopup.GetHost().HandleInput(event);
		return false;
	}

	if (m_projectSettingsPopup.IsOpen())
	{
		m_inspectorPanel.GetHost().Interrupt();
		m_projectSettingsPopup.GetHost().HandleInput(event);
		return event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseButton
			   || event.m_type == InputEventType::MouseWheel || event.m_type == InputEventType::Key
			   || event.m_type == InputEventType::Text;
	}

	if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
	{
		m_inspectorPanel.GetHost().Interrupt();
		return event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseButton
			   || event.m_type == InputEventType::MouseWheel || event.m_type == InputEventType::Key
			   || event.m_type == InputEventType::Text;
	}

	return m_inspectorPanel.GetHost().HandleInput(event);
}

bool EditorUI::IsNativeInputActive() const
{
	if (g_engine->m_imguiSystem == nullptr)
	{
		if (m_owner == nullptr)
		{
			return true;
		}

		Viewport* root = static_cast<Viewport*>(m_owner->GetSceneTree()->GetRoot());
		return root->GetGuiPopup() != nullptr || (root->GetGuiFocus() != nullptr && root->GetGuiFocus() != m_sceneView)
			   || (!IsSceneNavigationActive() && !IsSceneHovered());
	}

	Vector2 const cursor(g_engine->m_inputSystem->GetCursorClientPosition());
	return m_inspectorPanel.GetHost().HasKeyboardFocus() || m_projectSettingsPopup.GetHost().HasKeyboardFocus()
		   || m_inspectorPanel.GetHost().HasMouseCapture() || m_projectSettingsPopup.GetHost().HasMouseCapture()
		   || m_inspectorPanel.GetHost().Contains(cursor) || m_projectSettingsPopup.GetHost().Contains(cursor);
}

void EditorUI::FinishInput(EditorUIContext& context)
{
	m_inspectorPanel.FinishInput(context);
}

void EditorUI::Warning(std::string const& title, std::string const& message)
{
	if (g_engine->m_imguiSystem == nullptr)
	{
		ShowNativePopup(1, String(title + ": " + message));
		return;
	}

	m_warningPopup.Open(title, message);
}

void EditorUI::SetViewportRect(Vector2 origin, Vector2 dims)
{
	m_viewportOrigin = origin;
	m_viewportDims   = dims;
}

Vector2 EditorUI::GetViewportOrigin() const
{
	return m_viewportOrigin;
}

Vector2 EditorUI::GetViewportDimensions() const
{
	return m_viewportDims;
}

Vector2 EditorUI::ToViewportPos(Vector2 windowPos) const
{
	return m_sceneView != nullptr ? m_sceneView->ToViewportPosition(windowPos) : windowPos - m_viewportOrigin;
}

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
			ERR_PRINT("Script system is unavailable.\n");
		}
		else
		{
			if (g_engine->m_scriptSystem->EnsureProjectSolution())
			{
				if (g_engine->m_scriptSystem->BuildProjectSolution())
				{
					g_engine->m_scriptSystem->ReloadProjectAssembly();
				}
			}
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
		if (g_app != nullptr)
		{
			g_app->LaunchGame();
		}
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

#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"


Control* EditorUI::GetRootControl() const
{
	return static_cast<Control*>(m_nativeRoot);
}

bool EditorUI::HandleInput(InputEvent const& event)
{
	if (m_owner == nullptr)
	{
		return false;
	}

	if (event.m_type == InputEventType::WindowFocus && !event.m_focused)
	{
		m_viewportPanel->CancelNavigation();
	}

	UpdateNativeDocking();
	m_owner->GetSceneTree()->FlushContainerUpdate();
	auto* root = static_cast<Viewport*>(m_owner->GetSceneTree()->GetRoot());
	root->ProcessInputEvent(event);
	UpdateNativeDocking();
	root->UpdateGuiDrag(root->GetGuiDragPosition());
	return true;
}

bool EditorUI::IsNativeInputActive() const
{
	if (m_owner == nullptr)
	{
		return true;
	}

	auto* root = static_cast<Viewport*>(m_owner->GetSceneTree()->GetRoot());
	return root->GetGuiPopup() != nullptr || (root->GetGuiFocus() != nullptr && root->GetGuiFocus() != m_sceneView)
		|| (!IsSceneNavigationActive() && !IsSceneHovered());
}

void EditorUI::FinishInput(EditorUIContext& context)
{
	m_inspectorPanel->FinishInput(context);
}

void EditorUI::Warning(std::string const& title, std::string const& message)
{
	m_warningPopup.Open(title, message);
	if (m_nativeRoot != nullptr)
	{
		EditorUIContext context;
		context.m_rootControl = GetRootControl();
		context.m_sceneTree = m_owner->GetSceneTree();
		context.m_editorUI = this;
		m_warningPopup.Render(context);
	}
}

void EditorUI::SetViewportRect(Vector2 origin, Vector2 dims)
{
	m_viewportOrigin = origin;
	m_viewportDims = dims;
}

Vector2 EditorUI::ToViewportPos(Vector2 position) const
{
	return m_sceneView != nullptr ? m_sceneView->ToViewportPosition(position) : position - m_viewportOrigin;
}

bool EditorUI::IsSceneHovered() const
{
	if (m_sceneView == nullptr || g_engine->m_inputSystem == nullptr)
	{
		return false;
	}

	Vector2 const position(g_engine->m_inputSystem->GetCursorClientPosition());
	return m_sceneView->GetViewport()->GetGuiPopup() == nullptr && !m_sceneView->GetViewport()->IsGuiDragging()
		&& m_sceneView->IsVisibleInHierarchy() && m_sceneView->IsPointInside(position - m_sceneView->GetGlobalPosition());
}

bool EditorUI::IsSceneNavigationActive() const
{
	return m_viewportPanel != nullptr && m_viewportPanel->IsNavigating();
}

void EditorUI::TogglePanel(EditorPanel* panel)
{
	if (panel == m_viewportPanel)
	{
		return;
	}

	auto* slot = dynamic_cast<TabContainer*>(panel->GetParent());
	if (panel->IsOpen() && slot != nullptr)
	{
		slot->RemovePage(slot->GetPageIndex(panel));
		GetRootControl()->AddNode(panel);
		panel->Close();
	}
	else if (!panel->IsOpen())
	{
		int const destination = (panel == m_inspectorPanel || panel == m_importPanel) ? 2 : panel == m_outputPanel ? 3 : panel == m_fileSystemPanel ? 1 : 0;
		panel->Open();
		m_dockSlots[destination]->AddPage(panel->GetTitle(), panel);
	}

	UpdateNativeDocking();
	SaveNativeLayout();
}

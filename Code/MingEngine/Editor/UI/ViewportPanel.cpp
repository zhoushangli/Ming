#include "MingEngine/Editor/UI/ViewportPanel.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "ThirdParty/GLFW/glfw3.h"

ViewportPanel::ViewportPanel() : EditorPanel("Viewport")
{
	SetCustomMinimumSize(Vector2(400.f, 250.f));
}

void ViewportPanel::OnInitialize()
{
	auto* layout = MemNew<VBoxContainer>();
	AddNode(layout);
	layout->UpdateSize(GetSize());
	m_sceneTitle = EditorPopupUtils::AddLabel(layout, "");
	m_sceneTitle->SetClipText(true);
	m_sceneTitle->SetCustomMinimumSize(Vector2(0.f, 32.5f));
	m_sceneTitle->SetVerticalAlignment(VerticalAlignment::Center);
	auto* toolbar = MemNew<HBoxContainer>();
	toolbar->SetCustomMinimumSize(Vector2(0.f, 37.5f));
	layout->AddNode(toolbar);
	for (char const* icon : { "ToolSelect", "ToolMove", "ToolRotate", "ToolScale", "Snap" })
	{
		Button* button = EditorPopupUtils::AddButton(toolbar, "", {});
		button->SetName(icon);
		button->SetIcon(GetThemeIcon(icon, "EditorIcons"));
		button->SetThemeTypeVariation("EditorToolbarButton");
		button->SetDisabled(true);
	}

	m_view = MemNew<SubViewportContainer>();
	m_view->SetVerticalSizeFlags(Size_ExpandFill);
	m_view->SetInputCallback([this](InputEvent const& event) { return HandleSceneInput(event); });
	layout->AddNode(m_view);
}

void ViewportPanel::OnRender(EditorUIContext& context)
{
	m_sceneTitle->SetText(EditorNode::Get()->GetCurrentSceneName() + (EditorNode::Get()->IsSceneDirty() ? " *" : ""));
	context.m_editorUI->SetViewportRect(m_view->GetGlobalPosition(), m_view->GetSize());
}

bool ViewportPanel::HandleSceneInput(InputEvent const& event)
{
	EditorNode* owner = EditorNode::Get();
	switch (event.m_type)
	{
	case InputEventType::WindowFocus:
		if (!event.m_focused)
		{
			m_navigating = false;
		}
		break;
	case InputEventType::MouseMotion:
		owner->OnMouseMove(event.m_position + m_view->GetGlobalPosition(), event.m_relative);
		break;
	case InputEventType::MouseButton:
		if (event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_navigating = event.m_pressed && m_view->IsPointInside(event.m_position);
			if (m_navigating)
			{
				m_view->GrabFocus();
			}
		}
		else if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (event.m_pressed)
			{
				owner->OnMouseDown(ToKeyCode(KeyCode::LeftMouse), event.m_position + m_view->GetGlobalPosition());
			}
			else
			{
				owner->OnMouseUp(ToKeyCode(KeyCode::LeftMouse), event.m_position + m_view->GetGlobalPosition());
			}
		}
		break;
	}

	return event.m_type != InputEventType::Key;
}

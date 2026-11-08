#include "MingEngine/Editor/UI/Panels/ViewportPanel.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cmath>

ViewportPanel::ViewportPanel() : EditorPanel("Viewport")
{
}

void ViewportPanel::OnInitialize()
{
	this->AddThemeChangedCallback(
		[control = this]()
		{
			control->SetCustomMinimumSize(Vector2(
				static_cast<float>(control->GetThemeConstant("viewport_min_width", "Editor")),
				static_cast<float>(control->GetThemeConstant("viewport_min_height", "Editor"))));
		});
	auto* layout = MemNew<VBoxContainer>();
	AddNode(layout);
	layout->UpdateSize(GetSize());
	auto* header = MemNew<PanelContainer>();
	header->SetThemeTypeVariation("EditorViewportHeader");
	header->AddThemeChangedCallback(
		[control = header]()
		{
			control->SetCustomMinimumSize(
				Vector2(0.f, static_cast<float>(control->GetThemeConstant("viewport_header_height", "Editor"))));
		});
	layout->AddNode(header);
	auto* headerRow = MemNew<HBoxContainer>();
	header->AddNode(headerRow);
	auto* tab = MemNew<PanelContainer>();
	tab->SetThemeTypeVariation("EditorSceneTab");
	headerRow->AddNode(tab);
	auto* tabRow = MemNew<HBoxContainer>();
	tabRow->AddThemeChangedCallback(
		[control = tabRow]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("status_spacing", "Editor")) * 0.5f);
		});
	tab->AddNode(tabRow);
	auto* sceneIcon = MemNew<TextureRect>();
	sceneIcon->SetTexture(GetThemeIcon("PackedScene", "EditorIcons"));
	sceneIcon->SetTint(GetThemeColor("tn", "EditorIcons"));
	sceneIcon->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	sceneIcon->SetStretchMode(TextureRect::StretchMode::KeepAspectCentered);
	sceneIcon->AddThemeChangedCallback(
		[control = sceneIcon]()
		{
			control->SetCustomMinimumSize(Vector2(
				static_cast<float>(control->GetThemeConstant("icon_size", "Button")),
				static_cast<float>(control->GetThemeConstant("icon_size", "Button"))));
		});
	tabRow->AddNode(sceneIcon);
	m_sceneTitle = EditorPopupUtils::AddLabel(tabRow, "");
	m_sceneTitle->SetClipText(true);
	m_sceneTitle->AddThemeChangedCallback(
		[control = m_sceneTitle]()
		{
			control->SetCustomMinimumSize(
				Vector2(static_cast<float>(control->GetThemeConstant("scene_title_width", "Editor")), 0.f));
		});
	m_sceneTitle->SetHorizontalSizeFlags(Size_ExpandFill);
	m_sceneTitle->SetVerticalAlignment(VerticalAlignment::Center);
	auto addTool = [this](Control* parent, char const* name)
	{
		Button* button = EditorPopupUtils::AddButton(parent, "", {});
		button->SetName(name);
		button->SetIcon(GetThemeIcon(name, "EditorIcons"));
		button->SetThemeTypeVariation("EditorToolbarButton");
		button->SetDisabled(true);
		return button;
	};
	Button* close = addTool(tabRow, "Close");
	close->SetThemeTypeVariation("EditorPanelAction");
	close->SetDisabled(false);
	close->SetPressedCallback(
		[]()
		{
			EditorNode::Get()->RequestCloseScene();
		});
	Button* add = addTool(headerRow, "Add");
	add->SetThemeTypeVariation("EditorPanelAction");
	add->SetDisabled(false);
	add->SetPressedCallback(
		[]()
		{
			EditorNode::Get()->RequestNewScene();
		});
	auto* headerSpacer = MemNew<Control>();
	headerSpacer->SetHorizontalSizeFlags(Size_ExpandFill);
	headerRow->AddNode(headerSpacer);
	Button* more = addTool(headerRow, "More");
	more->SetThemeTypeVariation("EditorPanelAction");
	more->SetDisabled(false);
	more->SetPressedCallback(
		[this, more]()
		{
			m_context.m_editorUI->ShowMenu(
				more,
				more->GetGlobalPosition() + Vector2(0.f, more->GetSize().y),
				{ { "Grid",
					[]()
					{
						auto* gizmos = EditorNode::Get()->m_editorGizmos;
						gizmos->SetGridVisible(!gizmos->IsGridVisible());
					},
					"",
					false,
					true,
					EditorNode::Get()->m_editorGizmos->IsGridVisible() },
				  { "Reset View",
					[]()
					{
						EditorNode::Get()->GetEditorCamera()->ResetView();
					} } });
		});
	auto* toolbar = MemNew<HBoxContainer>();
	toolbar->AddThemeChangedCallback(
		[control = toolbar]()
		{
			control->SetCustomMinimumSize(
				Vector2(0.f, static_cast<float>(control->GetThemeConstant("viewport_toolbar_height", "Editor"))));
		});
	toolbar->AddThemeChangedCallback(
		[control = toolbar]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	layout->AddNode(toolbar);
	char const* icons[] = { "ToolSelect", "ToolMove", "ToolRotate", "ToolScale" };
	for (int i = 0; i < 4; ++i)
	{
		m_tools[i] = addTool(toolbar, icons[i]);
		m_tools[i]->SetDisabled(false);
		m_tools[i]->SetToggleMode(true);
		m_tools[i]->SetPressedCallback(
			[i]()
			{
				EditorNode::Get()->m_editorGizmos->SetTool(static_cast<GizmoTool>(i));
			});
	}

	auto* divider = MemNew<ColorRect>();
	divider->SetColor(GetThemeColor("separator_color", "MenuBar"));
	divider->AddThemeChangedCallback(
		[control = divider]()
		{
			control->SetCustomMinimumSize(
				Vector2(1.f, static_cast<float>(control->GetThemeConstant("separator_height", "Editor"))));
		});
	divider->SetVerticalSizeFlags(Size_ShrinkCenter);
	toolbar->AddNode(divider);
	m_grid = addTool(toolbar, "Grid");
	m_grid->SetDisabled(false);
	m_grid->SetToggleMode(true);
	m_grid->SetPressedCallback(
		[]()
		{
			auto* gizmos = EditorNode::Get()->m_editorGizmos;
			gizmos->SetGridVisible(!gizmos->IsGridVisible());
		});
	addTool(toolbar, "Snap");
	m_local = EditorPopupUtils::AddButton(
		toolbar,
		"World",
		[]()
		{
			auto* gizmos = EditorNode::Get()->m_editorGizmos;
			gizmos->SetLocal(!gizmos->IsLocal());
		});
	m_local->SetThemeTypeVariation("EditorToolbarButton");
	m_local->SetToggleMode(true);
	auto* spacer = MemNew<Control>();
	spacer->SetHorizontalSizeFlags(Size_ExpandFill);
	toolbar->AddNode(spacer);
	m_frame = MemNew<MarginContainer>();
	m_frame->SetMargins(0.f, 0.f, 0.f, 0.f);
	m_frame->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(m_frame);
	m_view = MemNew<SubViewportContainer>();
	m_view->SetVerticalSizeFlags(Size_ExpandFill);
	m_view->SetInputCallback(
		[this](InputEvent const& event)
		{
			return HandleSceneInput(event);
		});
	m_frame->AddNode(m_view);
}

void ViewportPanel::OnRender(EditorUIContext& context)
{
	std::string const name = EditorNode::Get()->GetCurrentSceneName();
	m_sceneTitle->SetText((name.empty() ? "Untitled" : name) + (EditorNode::Get()->IsSceneDirty() ? " *" : ""));
	EditorGizmos* gizmos = EditorNode::Get()->m_editorGizmos;
	for (int i = 0; i < 4; ++i)
	{
		m_tools[i]->SetPressedNoSignal(static_cast<int>(gizmos->GetTool()) == i);
		m_tools[i]->SetDisabled(gizmos->IsDragging());
	}

	m_local->SetText(gizmos->IsLocal() ? "Local" : "World");
	m_local->SetPressedNoSignal(gizmos->IsLocal());
	m_local->SetDisabled(gizmos->IsDragging());
	m_grid->SetPressedNoSignal(gizmos->IsGridVisible());
	if (m_view->GetViewport()->GetGuiPopup() != nullptr)
	{
		gizmos->CancelDrag();
		m_navigating = false;
	}
	else if (!context.m_editorUI->IsSceneHovered())
	{
		gizmos->ClearHover();
	}

	bool const compact = GetSize().x < 525.f;
	m_local->SetVisible(!compact);
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
			owner->m_editorGizmos->CancelDrag();
		}
		break;
	case InputEventType::MouseMotion:
		if (!m_navigating)
		{
			owner->OnMouseMove(event.m_position + m_view->GetGlobalPosition(), event.m_relative);
		}
		break;
	case InputEventType::MouseButton:
		if (event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
		{
			if (event.m_pressed && owner->m_editorGizmos->IsDragging())
			{
				owner->m_editorGizmos->CancelDrag();
				return true;
			}
			m_navigating =
				event.m_pressed && !owner->m_editorGizmos->IsDragging() && m_view->IsPointInside(event.m_position);
			if (m_navigating)
			{
				m_view->GrabFocus();
			}
		}
		else if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (event.m_pressed && !m_navigating)
			{
				m_view->GrabFocus();
				owner->OnMouseDown(ToKeyCode(KeyCode::LeftMouse), event.m_position + m_view->GetGlobalPosition());
			}
			else
			{
				owner->OnMouseUp(ToKeyCode(KeyCode::LeftMouse), event.m_position + m_view->GetGlobalPosition());
			}
		}
		break;
	case InputEventType::Key:
	{
		if (!event.m_pressed)
		{
			return false;
		}

		if (event.m_keyCode == GLFW_KEY_ESCAPE && owner->m_editorGizmos->IsDragging())
		{
			owner->m_editorGizmos->CancelDrag();
			return true;
		}

		if (m_navigating || owner->m_editorGizmos->IsDragging())
		{
			return false;
		}

		if ((event.m_modifiers & GLFW_MOD_CONTROL) != 0)
		{
			Node* scene = owner->GetSceneTree()->GetScene();
			if (scene != nullptr && (event.m_keyCode == GLFW_KEY_Z || event.m_keyCode == GLFW_KEY_Y))
			{
				bool const redo = event.m_keyCode == GLFW_KEY_Y || (event.m_modifiers & GLFW_MOD_SHIFT) != 0;
				if (redo)
				{
					owner->GetUndoRedo().Redo(scene->GetObjectID());
				}
				else
				{
					owner->GetUndoRedo().Undo(scene->GetObjectID());
				}
				return true;
			}
			return false;
		}

		switch (event.m_keyCode)
		{
		case GLFW_KEY_Q:
			owner->m_editorGizmos->SetTool(GizmoTool::Transform);
			return true;
		case GLFW_KEY_W:
			owner->m_editorGizmos->SetTool(GizmoTool::Move);
			return true;
		case GLFW_KEY_E:
			owner->m_editorGizmos->SetTool(GizmoTool::Rotate);
			return true;
		case GLFW_KEY_R:
			owner->m_editorGizmos->SetTool(GizmoTool::Scale);
			return true;
		}
		break;
	}
	}

	return event.m_type != InputEventType::Key;
}

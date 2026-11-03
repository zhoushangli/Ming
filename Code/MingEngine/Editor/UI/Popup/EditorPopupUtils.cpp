#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>

PopupPanel* EditorPopupUtils::Create(Control* owner, String const& title, VBoxContainer*& content)
{
	auto* popup = MemNew<PopupPanel>();
	popup->SetSerializable(false);
	popup->SetModal(true);
	owner->GetViewport()->AddNode(popup);
	popup->SetThemeSource(owner);
	auto* panel = MemNew<PanelContainer>();
	panel->SetClipContents(true);
	panel->SetThemeTypeVariation(title.IsEmpty() ? "EditorMenuPopup" : "EditorDialog");
	popup->AddNode(panel);
	Control* body = panel;
	if (!title.IsEmpty())
	{
		auto* shell = MemNew<VBoxContainer>();
		shell->SetSeparation(0.f);
		panel->AddNode(shell);
		auto* header = MemNew<PanelContainer>();
		header->SetThemeTypeVariation("EditorDialogHeader");
		shell->AddNode(header);
		auto* row = MemNew<HBoxContainer>();
		header->AddNode(row);
		Label* heading = AddLabel(row, title, "EditorInspectorTitle");
		heading->SetVerticalAlignment(VerticalAlignment::Center);
		heading->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		Button* close = AddButton(
			row,
			"",
			[popup]()
			{
				popup->Close();
			});
		close->SetName("CloseDialog");
		close->SetIcon(owner->GetThemeIcon("Close", "EditorIcons"));
		close->SetThemeTypeVariation("EditorPanelAction");
		auto* area = MemNew<PanelContainer>();
		area->SetThemeTypeVariation("EditorDialogBody");
		area->SetVerticalSizeFlags(Control::Size_ExpandFill);
		shell->AddNode(area);
		body = area;
	}

	content = MemNew<VBoxContainer>();
	content->AddThemeChangedCallback(
		[control = content]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("content_padding", "Editor")));
		});
	body->AddNode(content);

	ObjectID const id = popup->GetObjectID();
	popup->SetKeyCallback(
		[id](InputEvent const& event)
		{
			if (event.m_type == InputEventType::Key && event.m_pressed && event.m_keyCode == GLFW_KEY_ESCAPE)
			{
				if (auto* current = ObjectDatabase::GetInstance<PopupPanel>(id))
				{
					current->Close();
				}
			}
			else if (auto* current = ObjectDatabase::GetInstance<PopupPanel>(id))
			{
				Control* focus = current->GetViewport()->GetGuiFocus();
				if (focus != nullptr && focus != current)
				{
					focus->GuiInput(event);
				}
			}
		});
	return popup;
}

void EditorPopupUtils::Layout(PopupPanel* popup, Vector2 preferredSize)
{
	if (popup == nullptr || popup->GetChildren().empty() || popup->GetViewport() == nullptr)
	{
		return;
	}

	auto*         panel = static_cast<Control*>(popup->GetChildren().front());
	Vector2 const bounds(popup->GetViewport()->GetOutputResolution());
	Vector2 const minimum      = panel->GetCombinedMinimumSize();
	float const   screenMargin = static_cast<float>(popup->GetThemeConstant("popup_screen_margin", "Editor"));
	Vector2       size(
		std::min(std::max(preferredSize.x, minimum.x), std::max(1.f, bounds.x - screenMargin)),
		std::min(std::max(preferredSize.y, minimum.y), std::max(1.f, bounds.y - screenMargin)));
	panel->SetPosition((bounds - size) * 0.5f);
	panel->UpdateSize(size);
}

void EditorPopupUtils::Open(PopupPanel* popup, Control* owner, Vector2 preferredSize)
{
	popup->Open(owner);
	Layout(popup, preferredSize);
	owner->GetSceneTree()->FlushContainerUpdate();
	std::function<Button*(Node*)> findButton = [&](Node* node) -> Button*
	{
		if (auto* button = dynamic_cast<Button*>(node); button != nullptr && button->IsVisibleInHierarchy()
														&& !button->IsDisabled()
														&& button->GetName() != String("CloseDialog"))
		{
			return button;
		}

		for (Node* child : node->GetChildren())
		{
			if (Button* button = findButton(child))
			{
				return button;
			}
		}

		return nullptr;
	};
	if (Button* button = findButton(popup))
	{
		button->GrabFocus();
	}
}

Button* EditorPopupUtils::AddButton(Control* parent, String const& text, std::function<void()> callback)
{
	auto* button = MemNew<Button>();
	button->SetText(text);
	button->SetPressedCallback(std::move(callback));
	parent->AddNode(button);
	return button;
}

HBoxContainer* EditorPopupUtils::AddActions(Control* parent)
{
	auto* row = MemNew<HBoxContainer>();
	parent->AddNode(row);
	auto* spacer = MemNew<Control>();
	spacer->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	row->AddNode(spacer);
	return row;
}

Label* EditorPopupUtils::AddLabel(Control* parent, String const& text, std::string const& variation)
{
	auto* label = MemNew<Label>();
	label->SetText(text);
	if (!variation.empty())
	{
		label->SetThemeTypeVariation(variation);
	}

	parent->AddNode(label);
	return label;
}

void EditorPopupUtils::ClearContent(Control* content)
{
	if (content == nullptr)
	{
		return;
	}

	auto const children = content->GetChildren();
	for (Node* child : children)
	{
		if (auto* control = dynamic_cast<Control*>(child))
		{
			control->SetVisible(false);
		}

		child->QueueFree();
	}
}

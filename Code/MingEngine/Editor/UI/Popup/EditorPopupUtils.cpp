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
	auto* panel = MemNew<PanelContainer>();
	panel->SetClipContents(true);
	popup->AddNode(panel);
	content = MemNew<VBoxContainer>();
	content->SetSeparation(static_cast<float>(owner->GetThemeConstant("content_padding", "Editor")));
	panel->AddNode(content);
	if (!title.IsEmpty())
	{
		AddLabel(content, title, "EditorInspectorTitle");
	}

	ObjectID const id = popup->GetObjectID();
	popup->SetKeyCallback([id](InputEvent const& event)
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

	auto* panel = static_cast<Control*>(popup->GetChildren().front());
	Vector2 const bounds(popup->GetViewport()->GetOutputResolution());
	Vector2 const minimum = panel->GetCombinedMinimumSize();
	float const screenMargin = static_cast<float>(popup->GetThemeConstant("popup_screen_margin", "Editor"));
	Vector2 size(std::min(std::max(preferredSize.x, minimum.x), std::max(1.f, bounds.x - screenMargin)),
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
		if (auto* button = dynamic_cast<Button*>(node); button != nullptr && button->IsVisibleInHierarchy() && !button->IsDisabled())
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

#include "MingEngine/Scene/GUI/MenuBar.hpp"

#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>

MenuBar::MenuBar()
{
	SetSeparation(0.f);
	SetFocusMode(FocusMode::All);
}

MenuBar::~MenuBar()
{
	if (auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->SetKeyCallback({});
		popup->SetPointerCallback({});
		popup->SetClosedCallback({});
		popup->Close(false);
		for (Button* item : m_items)
		{
			item->SetPressedCallback({});
		}

		if (popup->GetSceneTree() != nullptr || popup->GetParent() == nullptr)
		{
			popup->QueueFree();
		}
	}
}

void MenuBar::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Constant, MenuBar, minimum_height);
	BIND_THEME_ITEM(Theme::DataType::Constant, MenuBar, popup_minimum_width);
}

Vector2 MenuBar::GetInternalMinimumSize() const
{
	Vector2 size = HBoxContainer::GetInternalMinimumSize();
	size.y       = std::max(size.y, static_cast<float>(m_themeCache.minimum_height));
	return size;
}

void MenuBar::AddMenu(String const& title, MenuProvider provider)
{
	auto* button = MemNew<Button>();
	button->SetText(title);
	button->SetThemeTypeVariation("MenuBarButton");
	button->SetToggleMode(true);
	button->SetFocusMode(FocusMode::None);
	int const index = static_cast<int>(m_menus.size());
	button->SetPressedCallback(
		[this, index]()
		{
			if (m_active == index)
			{
				CloseMenu();
			}
			else
			{
				OpenMenu(index);
			}
		});
	AddNode(button);
	m_menus.push_back({ button, std::move(provider) });
}

void MenuBar::ResetActiveMenu()
{
	m_active = -1;
	for (Menu const& menu : m_menus)
	{
		menu.m_button->SetPressedNoSignal(false);
	}
}

void MenuBar::CloseMenu(bool restoreFocus)
{
	if (auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->Close(restoreFocus);
	}

	ResetActiveMenu();
}

void MenuBar::OpenMenu(int index)
{
	if (GetViewport() == nullptr || index < 0 || index >= static_cast<int>(m_menus.size()))
	{
		return;
	}

	CloseMenu(false);
	if (m_aboutToOpen)
	{
		m_aboutToOpen();
	}

	auto* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID);
	if (popup == nullptr)
	{
		popup = MemNew<PopupPanel>();
		popup->SetSerializable(false);
		m_popupID = popup->GetObjectID();
		GetViewport()->AddNode(popup);
		m_popupPanel = MemNew<PanelContainer>();
		m_popupPanel->SetThemeTypeVariation("EditorMenuPopup");
		popup->AddNode(m_popupPanel);
		m_popupContent = MemNew<VBoxContainer>();
		m_popupPanel->AddNode(m_popupContent);
		popup->SetPointerCallback(
			[this](InputEvent const& event)
			{
				return HandlePopupPointer(event);
			});
		popup->SetKeyCallback(
			[this](InputEvent const& event)
			{
				HandlePopupKey(event);
			});
		popup->SetClosedCallback(
			[this]()
			{
				ResetActiveMenu();
			});
	}

	for (Node* item : m_popupContent->GetChildren())
	{
		static_cast<Control*>(item)->SetVisible(false);
		item->QueueFree();
	}

	m_items.clear();
	auto provider = m_menus[index].m_provider;
	m_items       = CreatePopupItems(
		m_popupContent,
		this,
		provider ? provider() : std::vector<MenuItem>(),
		[this]()
		{
			CloseMenu(false);
		});

	popup->Open(this);
	m_active   = index;
	m_selected = index;
	m_menus[index].m_button->SetPressedNoSignal(true);
	LayoutPopup();
	MoveItemFocus(1);
}

void MenuBar::LayoutPopup()
{
	if (m_active < 0 || GetViewport() == nullptr)
	{
		return;
	}

	Vector2 const bounds(GetViewport()->GetOutputResolution());
	Button*       button = m_menus[m_active].m_button;
	Vector2 const origin = button->GetGlobalPosition() + Vector2(0.f, button->GetSize().y);
	Vector2       size   = m_popupPanel->GetCombinedMinimumSize();
	size.x               = std::max(size.x, static_cast<float>(m_themeCache.popup_minimum_width));
	m_popupPanel->UpdateSize(size);
	m_popupPanel->SetPosition(Vector2(
		std::clamp(origin.x, 0.f, std::max(0.f, bounds.x - size.x)),
		std::clamp(origin.y, 0.f, std::max(0.f, bounds.y - size.y))));
	GetSceneTree()->FlushContainerUpdate();
	LayoutPopupItems(m_items);
}

std::vector<Button*> MenuBar::CreatePopupItems(
	VBoxContainer* content, Control* source, std::vector<MenuItem> const& items, std::function<void()> close,
	std::function<void(Button*, std::vector<MenuItem> const&)> openSubmenu)
{
	std::vector<Button*> buttons;
	for (MenuItem const& item : items)
	{
		if (item.m_separator)
		{
			auto* line = MemNew<ColorRect>();
			line->SetColor(source->GetThemeColor("separator_color", "MenuBar"));
			line->SetCustomMinimumSize(Vector2(0.f, static_cast<float>(source->GetThemeConstant("separator_width", "MenuBar"))));
			line->SetMouseFilter(MouseFilter::Ignore);
			content->AddNode(line);
			continue;
		}

		auto* button = MemNew<Button>();
		button->SetThemeSource(source);
		button->SetThemeTypeVariation("EditorMenuItem");
		button->SetHorizontalAlignment(HorizontalAlignment::Left);
		button->SetText(item.m_text);
		if (!item.m_icon.IsEmpty())
		{
			button->SetIcon(source->GetThemeIcon(item.m_icon.ToUtf8(), "EditorIcons"));
		}

		button->SetDisabled(!item.m_enabled || (!item.m_action && item.m_children.empty()));
		button->SetPressedCallback(
			[close, action = item.m_action, children = item.m_children, openSubmenu, button]()
			{
				if (!children.empty() && openSubmenu)
				{
					openSubmenu(button, children);
					return;
				}

				close();
				if (action)
				{
					action();
				}
			});
		content->AddNode(button);
		if (item.m_checkable && item.m_checked)
		{
			auto* check = MemNew<TextureRect>();
			check->SetName("Check");
			check->SetTexture(source->GetThemeIcon("Check", "EditorIcons"));
			check->SetTint(source->GetThemeColor("font_color", "Control"));
			check->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
			check->SetStretchMode(TextureRect::StretchMode::KeepAspectCentered);
			check->SetCustomMinimumSize(Vector2(
				static_cast<float>(source->GetThemeConstant("check_size", "MenuBar")),
				static_cast<float>(source->GetThemeConstant("check_size", "MenuBar"))));
			button->AddNode(check);
		}

		if (!item.m_shortcut.IsEmpty() || !item.m_children.empty())
		{
			auto* shortcut = MemNew<Label>();
			shortcut->SetName("Shortcut");
			shortcut->SetText(item.m_children.empty() ? item.m_shortcut : String("›"));
			shortcut->SetThemeTypeVariation("EditorStatusLabel");
			shortcut->SetHorizontalAlignment(HorizontalAlignment::Right);
			shortcut->SetVerticalAlignment(VerticalAlignment::Center);
			button->AddNode(shortcut);
			button->SetCustomMinimumSize(Vector2(
				button->GetCombinedMinimumSize().x + shortcut->GetCombinedMinimumSize().x
					+ source->GetThemeConstant("shortcut_separation", "MenuBar"),
				0.f));
		}

		buttons.push_back(button);
	}

	return buttons;
}

void MenuBar::LayoutPopupItems(std::vector<Button*> const& items)
{
	for (Button* item : items)
	{
		for (Node* child : item->GetChildren())
		{
			auto*         control = static_cast<Control*>(child);
			bool const    check   = control->GetName() == String("Check");
			Vector2 const size    = control->GetCombinedMinimumSize();
			control->SetPosition(Vector2(
				check ? item->GetThemeConstant("item_padding", "MenuBar")
					  : item->GetSize().x - size.x - item->GetThemeConstant("item_padding", "MenuBar"),
				check ? (item->GetSize().y - size.y) * 0.5f : 0.f));
			control->UpdateSize(Vector2(size.x, check ? size.y : item->GetSize().y));
		}
	}
}

void MenuBar::MoveItemFocus(int direction)
{
	int const count = static_cast<int>(m_items.size());
	int       index = direction > 0 ? -1 : 0;
	for (int i = 0; i < count; ++i)
	{
		if (m_items[i] == GetViewport()->GetGuiFocus())
		{
			index = i;
			break;
		}
	}

	for (int i = 0; i < count; ++i)
	{
		index = (index + direction + count) % count;
		if (!m_items[index]->IsDisabled())
		{
			m_items[index]->GrabFocus();
			return;
		}
	}
}

bool MenuBar::HandlePopupPointer(InputEvent const& event)
{
	for (int i = 0; i < static_cast<int>(m_menus.size()); ++i)
	{
		Button* button = m_menus[i].m_button;
		if (!button->IsPointInside(event.m_position - button->GetGlobalPosition()))
		{
			continue;
		}

		if (event.m_type == InputEventType::MouseMotion && i != m_active)
		{
			OpenMenu(i);
		}
		else if (
			event.m_type == InputEventType::MouseButton && event.m_pressed
			&& event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (i == m_active)
			{
				CloseMenu();
			}
			else
			{
				OpenMenu(i);
			}
		}

		return true;
	}

	return false;
}

void MenuBar::HandlePopupKey(InputEvent const& event)
{
	if (event.m_type == InputEventType::Key && event.m_pressed)
	{
		switch (event.m_keyCode)
		{
		case GLFW_KEY_ESCAPE:
			CloseMenu();
			return;
		case GLFW_KEY_LEFT:
		case GLFW_KEY_RIGHT:
			OpenMenu(
				(m_active + (event.m_keyCode == GLFW_KEY_RIGHT ? 1 : static_cast<int>(m_menus.size()) - 1))
				% static_cast<int>(m_menus.size()));
			return;
		case GLFW_KEY_UP:
		case GLFW_KEY_DOWN:
			MoveItemFocus(event.m_keyCode == GLFW_KEY_DOWN ? 1 : -1);
			return;
		}
	}

	Control* focus = GetViewport()->GetGuiFocus();
	if (std::find(m_items.begin(), m_items.end(), focus) != m_items.end())
	{
		focus->GuiInput(event);
	}
}

bool MenuBar::GuiInput(InputEvent const& event)
{
	switch (event.m_type)
	{
	case InputEventType::Key:
		if (event.m_pressed && !m_menus.empty())
		{
			switch (event.m_keyCode)
			{
			case GLFW_KEY_ENTER:
			case GLFW_KEY_KP_ENTER:
			case GLFW_KEY_SPACE:
			case GLFW_KEY_DOWN:
				OpenMenu(m_selected);
				return true;
			case GLFW_KEY_LEFT:
			case GLFW_KEY_RIGHT:
				m_selected =
					(m_selected + (event.m_keyCode == GLFW_KEY_RIGHT ? 1 : static_cast<int>(m_menus.size()) - 1))
					% static_cast<int>(m_menus.size());
				QueueRedraw();
				return true;
			}
		}
		break;
	}

	return false;
}

void MenuBar::OnThemeChanged()
{
	MarkDirty();
	LayoutPopup();
}

void MenuBar::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_ExitTree:
	case Notification_GuiInputCancelled:
		CloseMenu(false);
		break;
	case Notification_Resized:
		LayoutPopup();
		break;
	case Notification_FocusEnter:
	case Notification_FocusExit:
		QueueRedraw();
		break;
	case Notification_Draw:
		if (HasFocus() && !m_menus.empty())
		{
			Button* button = m_menus[m_selected].m_button;
			DrawThemeStyleBox(
				GetThemeStyleBox("focus", "MenuBarButton"),
				AABB2(button->GetLocalPosition(), button->GetLocalPosition() + button->GetSize()));
		}
		break;
	}
}

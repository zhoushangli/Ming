#include "MingEngine/Scene/GUI/MenuBar.hpp"

#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
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
	size.y = std::max(size.y, static_cast<float>(m_themeCache.minimum_height));
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
	button->SetPressedCallback([this, index]()
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
		popup->AddNode(m_popupPanel);
		m_popupContent = MemNew<VBoxContainer>();
		m_popupPanel->AddNode(m_popupContent);
		popup->SetPointerCallback([this](InputEvent const& event) { return HandlePopupPointer(event); });
		popup->SetKeyCallback([this](InputEvent const& event) { HandlePopupKey(event); });
		popup->SetClosedCallback([this]() { ResetActiveMenu(); });
	}

	for (Button* item : m_items)
	{
		item->SetVisible(false);
		item->SetPressedCallback({});
		item->QueueFree();
	}

	m_items.clear();
	auto provider = m_menus[index].m_provider;
	for (MenuItem const& item : provider ? provider() : std::vector<MenuItem>())
	{
		auto* button = MemNew<Button>();
		button->SetText(item.first);
		button->SetDisabled(!item.second);
		button->SetPressedCallback([this, action = item.second]()
		{
			CloseMenu(false);
			if (action)
			{
				action();
			}
		});
		m_popupContent->AddNode(button);
		m_items.push_back(button);
	}

	popup->Open(this);
	m_active = index;
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
	Button* button = m_menus[m_active].m_button;
	Vector2 const origin = button->GetGlobalPosition() + Vector2(0.f, button->GetSize().y);
	Vector2 size = m_popupPanel->GetCombinedMinimumSize();
	size.x = std::max(size.x, static_cast<float>(m_themeCache.popup_minimum_width));
	m_popupPanel->UpdateSize(size);
	m_popupPanel->SetPosition(Vector2(std::clamp(origin.x, 0.f, std::max(0.f, bounds.x - size.x)),
		std::clamp(origin.y, 0.f, std::max(0.f, bounds.y - size.y))));
	GetSceneTree()->FlushContainerUpdate();
}

void MenuBar::MoveItemFocus(int direction)
{
	int const count = static_cast<int>(m_items.size());
	int index = direction > 0 ? -1 : 0;
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
		else if (event.m_type == InputEventType::MouseButton && event.m_pressed
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
			OpenMenu((m_active + (event.m_keyCode == GLFW_KEY_RIGHT ? 1 : static_cast<int>(m_menus.size()) - 1))
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
				m_selected = (m_selected + (event.m_keyCode == GLFW_KEY_RIGHT ? 1 : static_cast<int>(m_menus.size()) - 1))
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
			DrawThemeStyleBox(GetThemeStyleBox("focus", "MenuBarButton"),
				AABB2(button->GetLocalPosition(), button->GetLocalPosition() + button->GetSize()));
		}
		break;
	}
}

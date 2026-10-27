#include "MingEngine/Scene/GUI/OptionButton.hpp"

#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include <algorithm>

OptionButton::OptionButton()
{
	SetPressedCallback(
		[this]()
		{
			OpenOptions();
		});
}

OptionButton::~OptionButton()
{
	if (PopupPanel* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->SetKeyCallback({});
		popup->Close(false);
		for (Button* button : m_buttons)
		{
			button->SetPressedCallback({});
		}

		if (popup->GetSceneTree() != nullptr || popup->GetParent() == nullptr)
		{
			popup->QueueFree();
		}
	}
}

bool OptionButton::SetOptions(std::string const& hint)
{
	CloseOptions(false);
	if (PopupPanel* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->SetKeyCallback({});
		popup->QueueFree();
	}

	m_popupID = ObjectID::Invalid;
	m_buttons.clear();
	m_items.clear();
	size_t start = 0;
	while (start < hint.size())
	{
		size_t const      end    = hint.find(',', start);
		std::string const option = hint.substr(start, end == std::string::npos ? end : end - start);
		size_t const      colon  = option.find(':');
		Variant           value;
		if (colon == std::string::npos || colon == 0 || !SpinBox::ParseValue(option.substr(colon + 1), true, value))
		{
			m_items.clear();
			break;
		}

		m_items.push_back({ String(option.substr(0, colon)), value.As<int>() });
		if (end == std::string::npos)
		{
			break;
		}

		start = end + 1;
		if (start == hint.size())
		{
			m_items.clear();
		}
	}

	SetDisabled(m_items.empty());
	SetValueNoSignal(m_value);
	return !m_items.empty();
}

void OptionButton::SetValueNoSignal(int value)
{
	m_value = value;
	SetText(std::to_string(value));
	for (Item const& item : m_items)
	{
		if (item.m_value == value)
		{
			SetText(item.m_name);
			break;
		}
	}
}

void OptionButton::OpenOptions()
{
	if (GetViewport() == nullptr || m_items.empty())
	{
		return;
	}

	PopupPanel* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID);
	if (popup == nullptr)
	{
		popup = MemNew<PopupPanel>();
		popup->SetSerializable(false);
		m_popupID = popup->GetObjectID();
		GetViewport()->AddNode(popup);
		m_scroll = MemNew<ScrollContainer>();
		popup->AddNode(m_scroll);
		auto* list = MemNew<VBoxContainer>();
		list->SetSeparation(1.f);
		m_scroll->AddNode(list);
		for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
		{
			auto* button = MemNew<Button>();
			button->SetFont(GetFont());
			button->SetFontSize(GetFontSize());
			button->SetToggleMode(true);
			button->SetFocusMode(FocusMode::None);
			button->SetText(m_items[i].m_name);
			button->SetPressedCallback(
				[this, i]()
				{
					Select(i);
				});
			list->AddNode(button);
			m_buttons.push_back(button);
		}

		popup->SetKeyCallback(
			[this](InputEvent const& event)
			{
				HandleKey(event);
			});
	}

	m_active = 0;
	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		if (m_items[i].m_value == m_value)
		{
			m_active = i;
			break;
		}
	}

	Vector2 const bounds(GetViewport()->GetOutputResolution());
	Vector2 const origin = GetGlobalPosition();
	float const   height = std::min(bounds.y, std::min(240.f, static_cast<float>(m_items.size()) * 36.f));
	float const   width  = std::min(bounds.x, std::max(GetSize().x, 180.f));
	float const   y =
		origin.y + GetSize().y + height <= bounds.y ? origin.y + GetSize().y : std::max(0.f, origin.y - height);
	m_scroll->SetPosition(Vector2(std::clamp(origin.x, 0.f, std::max(0.f, bounds.x - width)), y));
	m_scroll->UpdateSize(Vector2(width, height));
	popup->Open(this);
	GetSceneTree()->FlushContainerUpdate();
	UpdateActive();
}

void OptionButton::CloseOptions(bool restoreFocus)
{
	if (PopupPanel* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID))
	{
		popup->Close(restoreFocus);
	}
}

void OptionButton::UpdateActive()
{
	for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i)
	{
		m_buttons[i]->SetPressedNoSignal(i == m_active);
	}

	if (!m_buttons.empty())
	{
		m_scroll->EnsureControlVisible(m_buttons[m_active]);
	}
}

void OptionButton::Select(int index)
{
	if (index < 0 || index >= static_cast<int>(m_items.size()))
	{
		return;
	}

	int const  value   = m_items[index].m_value;
	bool const changed = value != m_value;
	SetValueNoSignal(value);
	CloseOptions();
	auto callback = m_changed;
	if (changed && callback)
	{
		callback(value);
	}
}

void OptionButton::HandleKey(InputEvent const& event)
{
	if (event.m_type != InputEventType::Key || !event.m_pressed)
	{
		return;
	}

	if (event.m_keyCode == GLFW_KEY_ESCAPE || event.m_keyCode == GLFW_KEY_TAB)
	{
		CloseOptions();
	}
	else if (event.m_keyCode == GLFW_KEY_ENTER || event.m_keyCode == GLFW_KEY_KP_ENTER)
	{
		Select(m_active);
	}
	else if (event.m_keyCode == GLFW_KEY_DOWN || event.m_keyCode == GLFW_KEY_UP)
	{
		int const count = static_cast<int>(m_items.size());
		m_active        = (m_active + (event.m_keyCode == GLFW_KEY_DOWN ? 1 : count - 1)) % count;
		UpdateActive();
	}
}

void OptionButton::OnNotification(int notification)
{
	if (notification == Notification_ExitTree || notification == Notification_GuiInputCancelled)
	{
		CloseOptions(false);
	}
}

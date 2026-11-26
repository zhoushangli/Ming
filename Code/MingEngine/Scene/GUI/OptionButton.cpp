#include "MingEngine/Scene/GUI/OptionButton.hpp"

#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include <algorithm>

OptionButton::OptionButton()
{
	SetHorizontalAlignment(HorizontalAlignment::Left);
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
	m_indicators.clear();
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
	if (PopupPanel* popup = ObjectDatabase::GetInstance<PopupPanel>(m_popupID); popup != nullptr && popup->IsVisible())
	{
		UpdateActive(false);
	}
}

Vector2 OptionButton::GetInternalMinimumSize() const
{
	return Button::GetInternalMinimumSize() + Vector2(
		static_cast<float>(GetThemeConstant("arrow_size", "OptionButton") + GetThemeConstant("arrow_margin", "OptionButton")), 0.f);
}

AABB2 OptionButton::GetTextRect() const
{
	AABB2 rect = Button::GetTextRect();
	rect.m_maxs.x = std::max(rect.m_mins.x, rect.m_maxs.x
		- GetThemeConstant("arrow_size", "OptionButton") - GetThemeConstant("arrow_margin", "OptionButton"));
	return rect;
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
		popup->SetThemeSource(this);
		m_popupPanel = MemNew<PanelContainer>();
		m_popupPanel->SetName("OptionPopup");
		m_popupPanel->SetThemeTypeVariation("OptionButtonPopup");
		m_popupPanel->SetMouseFilter(MouseFilter::Pass);
		popup->AddNode(m_popupPanel);
		m_scroll = MemNew<ScrollContainer>();
		m_scroll->SetFollowFocus(true);
		m_scroll->SetMouseFilter(MouseFilter::Pass);
		m_popupPanel->AddNode(m_scroll);
		auto* list = MemNew<VBoxContainer>();
		m_list = list;
		list->SetSeparation(0.f);
		list->SetMouseFilter(MouseFilter::Pass);
		m_scroll->AddNode(list);
		for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
		{
			auto* button = MemNew<Button>();
			button->SetThemeTypeVariation("OptionButtonItem");
			button->SetHorizontalAlignment(HorizontalAlignment::Left);
			button->SetMouseFilter(MouseFilter::Pass);
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
			auto* indicator = MemNew<PanelContainer>();
			indicator->SetName("OptionSelection");
			indicator->SetMouseFilter(MouseFilter::Ignore);
			button->AddNode(indicator);
			m_indicators.push_back(indicator);
		}
		popup->SetPointerCallback([this](InputEvent const& event)
		{
			if (event.m_type == InputEventType::MouseMotion
				&& m_scroll->IsPointInside(event.m_position - m_scroll->GetGlobalPosition()))
			{
				for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i)
				{
					if (m_buttons[i]->IsPointInside(event.m_position - m_buttons[i]->GetGlobalPosition()))
					{
						m_active = i;
						UpdateActive(false);
						break;
					}
				}
			}
			return false;
		});

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

	popup->Open(this);
	GetSceneTree()->FlushContainerUpdate();
	Vector2 const bounds(GetViewport()->GetOutputResolution());
	Vector2 const origin = GetGlobalPosition();
	auto panel = m_popupPanel->GetThemeStyleBox("panel");
	Vector2 const padding = panel.IsValid() ? panel->GetMinimumSize() : Vector2::Zero;
	Vector2 const content = m_list->GetCombinedMinimumSize() + padding;
	float const height = std::min(bounds.y, std::min(static_cast<float>(GetThemeConstant("popup_max_height", "OptionButton")), content.y));
	float const scrollbar = content.y > height ? static_cast<float>(m_scroll->GetThemeConstant("scrollbar_width", "ScrollContainer")) : 0.f;
	float const width = std::min(bounds.x, std::max(GetSize().x, content.x + scrollbar));
	float const   y =
		origin.y + GetSize().y + height <= bounds.y ? origin.y + GetSize().y : std::max(0.f, origin.y - height);
	m_popupPanel->SetPosition(Vector2(std::clamp(origin.x, 0.f, std::max(0.f, bounds.x - width)), y));
	m_popupPanel->UpdateSize(Vector2(width, height));
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

void OptionButton::UpdateActive(bool ensureVisible)
{
	for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i)
	{
		m_buttons[i]->SetPressedNoSignal(i == m_active);
		float const size = static_cast<float>(GetThemeConstant("radio_size", "OptionButton"));
		m_indicators[i]->AddThemeStyleBoxOverride("panel",
			GetThemeStyleBox(m_items[i].m_value == m_value ? "checked" : "unchecked", "OptionButtonRadio"));
		m_indicators[i]->SetPosition(Vector2(
			static_cast<float>(GetThemeConstant("radio_margin", "OptionButton")),
			std::max(0.f, (m_buttons[i]->GetSize().y - size) * 0.5f)));
		m_indicators[i]->UpdateSize(Vector2(size, size));
	}

	if (ensureVisible && !m_buttons.empty())
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
	switch (notification)
	{
	case Notification_ExitTree:
	case Notification_GuiInputCancelled:
		CloseOptions(false);
		break;
	case Notification_Draw:
	{
		float const size = static_cast<float>(GetThemeConstant("arrow_size", "OptionButton"));
		float const margin = static_cast<float>(GetThemeConstant("arrow_margin", "OptionButton"));
		Vector2 const center(GetSize().x - margin - size * 0.5f, GetSize().y * 0.5f);
		Color const color = GetThemeColor(IsDisabled() ? "font_disabled_color" : "font_color");
		DrawTriangles({ Vertex(Vector3(center.x - size * 0.5f, center.y - size * 0.25f, 0.f), color),
			Vertex(Vector3(center.x + size * 0.5f, center.y - size * 0.25f, 0.f), color),
			Vertex(Vector3(center.x, center.y + size * 0.25f, 0.f), color) });
		break;
	}
	}
}

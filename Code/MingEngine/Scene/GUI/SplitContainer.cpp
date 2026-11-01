#include "MingEngine/Scene/GUI/SplitContainer.hpp"

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cmath>

SplitContainer::SplitContainer()
{
	SetClipContents(true);
	SetMouseFilter(MouseFilter::Stop);
}

void SplitContainer::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Constant, SplitContainer, separation);
	BIND_THEME_ITEM(Theme::DataType::Constant, SplitContainer, grabber_length);
	BIND_THEME_ITEM(Theme::DataType::Constant, SplitContainer, grabber_width);
	BIND_THEME_ITEM(Theme::DataType::Constant, SplitContainer, grabber_separation);
	BIND_THEME_ITEM(Theme::DataType::Color, SplitContainer, grabber_color);
	BIND_THEME_ITEM(Theme::DataType::Color, SplitContainer, grabber_hover_color);
	BIND_THEME_ITEM(Theme::DataType::Color, SplitContainer, grabber_pressed_color);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, SplitContainer, normal);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, SplitContainer, hover);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, SplitContainer, pressed);
}

void SplitContainer::OnThemeChanged()
{
	if (m_appliedSeparation != m_themeCache.separation)
	{
		m_dragging = false;
		m_hovering = false;
	}

	m_appliedSeparation = m_themeCache.separation;
	MarkDirty();
}

float SplitContainer::GetSeparation() const
{
	return static_cast<float>(std::max(0, m_themeCache.separation));
}

AABB2 SplitContainer::GetDividerRect() const
{
	Control* first;
	Control* second;
	GetPanels(first, second);
	if (first == nullptr || second == nullptr)
	{
		return AABB2::Zero;
	}

	return m_vertical ? AABB2(0.f, m_offset, GetSize().x, m_offset + GetSeparation())
					  : AABB2(m_offset, 0.f, m_offset + GetSeparation(), GetSize().y);
}

void SplitContainer::GetPanels(Control*& first, Control*& second) const
{
	first  = nullptr;
	second = nullptr;
	for (Node* child : GetChildren())
	{
		if (Control* pane = GetLayoutChild(child))
		{
			if (first == nullptr)
			{
				first = pane;
			}
			else
			{
				second = pane;
				break;
			}
		}
	}
}

Vector2 SplitContainer::GetInternalMinimumSize() const
{
	Control* first;
	Control* second;
	GetPanels(first, second);
	Vector2 const a   = first != nullptr ? first->GetCombinedMinimumSize() : Vector2::Zero;
	Vector2 const b   = second != nullptr ? second->GetCombinedMinimumSize() : Vector2::Zero;
	float const   gap = first != nullptr && second != nullptr ? GetSeparation() : 0.f;
	return m_vertical ? Vector2(std::max(a.x, b.x), a.y + b.y + gap) : Vector2(a.x + b.x + gap, std::max(a.y, b.y));
}

void SplitContainer::SetVertical(bool vertical)
{
	if (m_vertical != vertical)
	{
		m_vertical = vertical;
		m_dragging = false;
		m_hovering = false;
		PropagateMinimumSizeChanged();
		MarkDirty();
	}
}

void SplitContainer::SetSplitRatio(float ratio)
{
	if (!std::isfinite(ratio))
	{
		return;
	}

	m_ratio = std::clamp(ratio, 0.f, 1.f);
	MarkDirty();
}

bool SplitContainer::GuiInput(InputEvent const& event)
{
	Control* first;
	Control* second;
	GetPanels(first, second);
	float const cursor = m_vertical ? event.m_position.y : event.m_position.x;

	switch (event.m_type)
	{
	case InputEventType::MouseButton:
	{
		if (event.m_mouseButton != GLFW_MOUSE_BUTTON_LEFT)
		{
			break;
		}

		if (event.m_pressed && first != nullptr && second != nullptr && GetSeparation() > 0.f
			&& GetDividerRect().IsPointInside(event.m_position))
		{
			m_dragging = true;
			QueueRedraw();
			m_grabOffset = cursor - m_offset;
			return true;
		}

		if (!event.m_pressed && m_dragging)
		{
			m_dragging = false;
			QueueRedraw();
			auto callback = m_finished;
			if (callback)
			{
				callback();
			}

			return true;
		}

		break;
	}
	case InputEventType::MouseMotion:
	{
		AABB2 const divider = GetDividerRect();
		bool const  hovering =
			divider.GetWidth() > 0.f && divider.GetHeight() > 0.f && divider.IsPointInside(event.m_position);
		if (m_hovering != hovering)
		{
			m_hovering = hovering;
			QueueRedraw();
		}

		if (m_dragging)
		{
			float const length = std::max(1.f, (m_vertical ? GetSize().y : GetSize().x) - GetSeparation());
			SetSplitRatio((cursor - m_grabOffset) / length);
			return true;
		}
		break;
	}
	}

	return false;
}

CursorShape SplitContainer::GetCursorShape(Vector2 const& localPosition) const
{
	Control* first;
	Control* second;
	GetPanels(first, second);
	if (first != nullptr && second != nullptr && GetSeparation() > 0.f
		&& (m_dragging || GetDividerRect().IsPointInside(localPosition)))
	{
		return m_vertical ? CursorShape::ResizeVertical : CursorShape::ResizeHorizontal;
	}

	return CursorShape::Arrow;
}

void SplitContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_UpdateLayout:
	{
		Control* first;
		Control* second;
		GetPanels(first, second);
		if (first == nullptr)
		{
			return;
		}

		if (second == nullptr)
		{
			first->SetPosition(Vector2::Zero);
			first->UpdateSize(GetSize());
			m_offset = 0.f;

			QueueRedraw();

			return;
		}

		Vector2 const a       = first->GetCombinedMinimumSize();
		Vector2 const b       = second->GetCombinedMinimumSize();
		float const   length  = std::max(0.f, (m_vertical ? GetSize().y : GetSize().x) - GetSeparation());
		float const   minimum = std::min(length, m_vertical ? a.y : a.x);
		float const   maximum = std::max(minimum, length - (m_vertical ? b.y : b.x));
		m_offset              = std::clamp(length * m_ratio, minimum, maximum);
		first->SetPosition(Vector2::Zero);
		first->UpdateSize(m_vertical ? Vector2(GetSize().x, m_offset) : Vector2(m_offset, GetSize().y));
		second->SetPosition(
			m_vertical ? Vector2(0.f, m_offset + GetSeparation()) : Vector2(m_offset + GetSeparation(), 0.f));
		second->UpdateSize(
			m_vertical ? Vector2(GetSize().x, length - m_offset) : Vector2(length - m_offset, GetSize().y));

		QueueRedraw();

		break;
	}
	case Notification_MouseExit:
		m_hovering = false;
		QueueRedraw();
		break;
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		m_dragging = false;
		m_hovering = false;
		QueueRedraw();
		break;
	case Notification_Draw:
	{
		AABB2 const divider = GetDividerRect();
		if (divider.GetWidth() <= 0.f || divider.GetHeight() <= 0.f)
		{
			break;
		}

		DrawThemeStyleBox(
			m_dragging ? m_themeCache.pressed : (m_hovering ? m_themeCache.hover : m_themeCache.normal),
			divider);

		// Draw grabber
		float const available = m_vertical ? divider.GetWidth() : divider.GetHeight();
		float const length    = std::min(available, static_cast<float>(std::max(0, m_themeCache.grabber_length)));
		float const width     = static_cast<float>(std::max(0, m_themeCache.grabber_width));
		float const step      = std::max(1.f, width + static_cast<float>(std::max(0, m_themeCache.grabber_separation)));
		float const start     = (available - length) * 0.5f;
		Color const color = m_dragging ? m_themeCache.grabber_pressed_color
									   : (m_hovering ? m_themeCache.grabber_hover_color : m_themeCache.grabber_color);
		for (float offset = 0.f; width > 0.f && offset < length; offset += step)
		{
			float const end  = start + std::min(length, offset + width);
			AABB2 const tick = m_vertical ? AABB2(start + offset, divider.m_mins.y, end, divider.m_maxs.y)
										  : AABB2(divider.m_mins.x, start + offset, divider.m_maxs.x, end);
			DrawRect(tick, color);
		}
		break;
	}
	}
}

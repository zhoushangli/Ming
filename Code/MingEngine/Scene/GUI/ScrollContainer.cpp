#include "MingEngine/Scene/GUI/ScrollContainer.hpp"

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#include <algorithm>
#include <cmath>

ScrollContainer::ScrollContainer()
{
	SetClipContents(true);
	SetMouseFilter(MouseFilter::Stop);
}

void ScrollContainer::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Color, ScrollContainer, background_color);
	BIND_THEME_ITEM(Theme::DataType::Constant, ScrollContainer, scrollbar_width);
	BIND_THEME_ITEM(Theme::DataType::Constant, ScrollContainer, grabber_minimum_size);
	BIND_THEME_ITEM(Theme::DataType::Constant, ScrollContainer, grabber_margin);
	BIND_THEME_ITEM(Theme::DataType::Constant, ScrollContainer, scroll_step);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, ScrollContainer, track);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, ScrollContainer, grabber);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, ScrollContainer, grabber_hover);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, ScrollContainer, grabber_pressed);
}

void ScrollContainer::OnThemeChanged()
{
	if (m_dragAxis >= 0
		&& (m_dragScrollbarWidth != m_themeCache.scrollbar_width
			|| m_dragMinimumSize != m_themeCache.grabber_minimum_size || m_dragMargin != m_themeCache.grabber_margin))
	{
		m_dragAxis = -1;
	}

	Arrange();
	MarkDirty();
}

AABB2 ScrollContainer::GetTrackRect(int axis) const
{
	float const length = axis == 0 ? m_pageSize.x : m_pageSize.y;
	float const extent = axis == 0 ? m_contentSize.x : m_contentSize.y;
	if (extent <= length || length <= 0.f || m_themeCache.scrollbar_width <= 0)
	{
		return AABB2::Zero;
	}

	return axis == 0 ? AABB2(0.f, m_pageSize.y, length, GetSize().y) : AABB2(m_pageSize.x, 0.f, GetSize().x, length);
}

AABB2 ScrollContainer::GetGrabberRect(int axis) const
{
	AABB2 const track  = GetTrackRect(axis);
	float const length = axis == 0 ? track.GetWidth() : track.GetHeight();
	if (track.GetWidth() <= 0.f || track.GetHeight() <= 0.f)
	{
		return AABB2::Zero;
	}

	float const extent = axis == 0 ? m_contentSize.x : m_contentSize.y;
	float const offset = axis == 0 ? m_horizontalScroll : m_scroll;
	float const thumb  = std::min(
		length,
		std::max(static_cast<float>(std::max(1, m_themeCache.grabber_minimum_size)), length * length / extent));
	float const top       = offset / (extent - length) * (length - thumb);
	float const thickness = axis == 0 ? track.GetHeight() : track.GetWidth();
	float const margin =
		std::min(static_cast<float>(std::max(0, m_themeCache.grabber_margin)), std::max(0.f, (thickness - 1.f) * 0.5f));
	return axis == 0 ? AABB2(top, track.m_mins.y + margin, top + thumb, track.m_maxs.y - margin)
					 : AABB2(track.m_mins.x + margin, top, track.m_maxs.x - margin, top + thumb);
}

Control* ScrollContainer::GetContent() const
{
	for (Node* child : GetChildren())
	{
		if (Control* control = GetLayoutChild(child))
		{
			return control;
		}
	}

	return nullptr;
}

float ScrollContainer::GetMaximumScroll() const
{
	return std::max(0.f, m_contentSize.y - m_pageSize.y);
}

void ScrollContainer::Arrange()
{
	Control*      content         = GetContent();
	Vector2 const minimum         = content != nullptr ? content->GetCombinedMinimumSize() : Vector2::Zero;
	Vector2 const previousPage    = m_pageSize;
	Vector2 const previousContent = m_contentSize;
	float const   thickness       = static_cast<float>(std::max(0, m_themeCache.scrollbar_width));
	bool          horizontal      = false;
	bool          vertical        = false;
	for (int i = 0; i < 3; ++i)
	{
		m_pageSize = Vector2(
			std::max(0.f, GetSize().x - (vertical ? thickness : 0.f)),
			std::max(0.f, GetSize().y - (horizontal ? thickness : 0.f)));
		horizontal = minimum.x > m_pageSize.x;
		vertical   = minimum.y > m_pageSize.y;
	}

	m_pageSize = Vector2(
		std::max(0.f, GetSize().x - (vertical ? thickness : 0.f)),
		std::max(0.f, GetSize().y - (horizontal ? thickness : 0.f)));
	m_contentSize = Vector2(std::max(minimum.x, m_pageSize.x), std::max(minimum.y, m_pageSize.y));
	if (m_pageSize != previousPage || m_contentSize != previousContent)
	{
		m_dragAxis  = -1;
		m_hoverAxis = -1;
	}

	m_scroll           = std::clamp(m_scroll, 0.f, GetMaximumScroll());
	m_horizontalScroll = std::clamp(m_horizontalScroll, 0.f, std::max(0.f, m_contentSize.x - m_pageSize.x));
	if (content != nullptr)
	{
		Rect2 rect;
		rect.SetPosition(Vector2(-m_horizontalScroll, -m_scroll));
		rect.SetSize(m_contentSize);
		UpdateChildRect(content, rect);
	}

	SetChildrenClip(true, m_pageSize);
	QueueRedraw();
}

void ScrollContainer::SetScroll(float offset)
{
	if (!std::isfinite(offset))
	{
		return;
	}

	m_scroll = std::max(0.f, offset);
	Arrange();
}

void ScrollContainer::SetHorizontalScroll(float offset)
{
	if (!std::isfinite(offset))
	{
		return;
	}

	m_horizontalScroll = std::max(0.f, offset);
	Arrange();
}

void ScrollContainer::EnsureControlVisible(Control* control)
{
	if (control == nullptr)
	{
		return;
	}

	Vector2 const top    = control->GetGlobalPosition() - GetGlobalPosition();
	Vector2 const bottom = top + control->GetSize();
	if (top.y < 0.f)
	{
		m_scroll += top.y;
	}
	else if (bottom.y > m_pageSize.y)
	{
		m_scroll += bottom.y - m_pageSize.y;
	}

	if (top.x < 0.f)
	{
		m_horizontalScroll += top.x;
	}
	else if (bottom.x > m_pageSize.x && control->GetSize().x <= m_pageSize.x)
	{
		m_horizontalScroll += bottom.x - m_pageSize.x;
	}

	Arrange();
}

bool ScrollContainer::GuiInput(InputEvent const& event)
{
	if (event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseButton)
	{
		int hovered = -1;
		for (int axis = 0; axis < 2; ++axis)
		{
			AABB2 const thumb = GetGrabberRect(axis);
			if (thumb.GetWidth() > 0.f && thumb.GetHeight() > 0.f && thumb.IsPointInside(event.m_position))
			{
				hovered = axis;
			}
		}

		if (hovered != m_hoverAxis)
		{
			m_hoverAxis = hovered;
			QueueRedraw();
		}
	}

	switch (event.m_type)
	{
	case InputEventType::MouseWheel:
	{
		float const step = static_cast<float>(std::max(0, m_themeCache.scroll_step));
		if ((event.m_modifiers & GLFW_MOD_SHIFT) != 0)
		{
			SetHorizontalScroll(m_horizontalScroll - event.m_scroll.y * step);
		}
		else
		{
			m_horizontalScroll -= event.m_scroll.x * step;
			SetScroll(m_scroll - event.m_scroll.y * step);
		}

		return true;
	}
	case InputEventType::MouseButton:
	{
		if (event.m_mouseButton != GLFW_MOUSE_BUTTON_LEFT)
		{
			break;
		}

		if (!event.m_pressed)
		{
			m_dragAxis = -1;
			QueueRedraw();
			return true;
		}

		m_dragAxis = -1;
		for (int axis = 0; axis < 2; ++axis)
		{
			AABB2 const track = GetTrackRect(axis);
			if (track.GetWidth() > 0.f && track.GetHeight() > 0.f && track.IsPointInside(event.m_position))
			{
				m_dragAxis = axis;
				break;
			}
		}

		if (m_dragAxis < 0)
		{
			return false;
		}

		AABB2 const thumb    = GetGrabberRect(m_dragAxis);
		float const top      = m_dragAxis == 0 ? thumb.m_mins.x : thumb.m_mins.y;
		float const size     = m_dragAxis == 0 ? thumb.GetWidth() : thumb.GetHeight();
		float const position = m_dragAxis == 0 ? event.m_position.x : event.m_position.y;
		m_dragOffset         = position >= top && position < top + size ? position - top : size * 0.5f;
		m_dragScrollbarWidth = m_themeCache.scrollbar_width;
		m_dragMinimumSize    = m_themeCache.grabber_minimum_size;
		m_dragMargin         = m_themeCache.grabber_margin;
		QueueRedraw();
	}
		[[fallthrough]];
	case InputEventType::MouseMotion:
	{
		if (m_dragAxis < 0)
		{
			break;
		}

		AABB2 const track    = GetTrackRect(m_dragAxis);
		AABB2 const thumb    = GetGrabberRect(m_dragAxis);
		float const length   = m_dragAxis == 0 ? track.GetWidth() : track.GetHeight();
		float const size     = m_dragAxis == 0 ? thumb.GetWidth() : thumb.GetHeight();
		float const extent   = m_dragAxis == 0 ? m_contentSize.x : m_contentSize.y;
		float const position = m_dragAxis == 0 ? event.m_position.x : event.m_position.y;
		float const offset   = length > size ? (position - m_dragOffset) / (length - size) * (extent - length) : 0.f;
		if (m_dragAxis == 0)
		{
			SetHorizontalScroll(offset);
		}
		else
		{
			SetScroll(offset);
		}

		return true;
	}
	}

	return false;
}

void ScrollContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_UpdateLayout:
		Arrange();
		break;
	case Notification_MouseExit:
		m_hoverAxis = -1;
		QueueRedraw();
		break;
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		m_dragAxis  = -1;
		m_hoverAxis = -1;
		QueueRedraw();
		break;
	case Notification_Draw:
		DrawRect(GetLocalRect(), m_themeCache.background_color);
		for (int axis = 0; axis < 2; ++axis)
		{
			AABB2 const track = GetTrackRect(axis);
			if (track.GetWidth() <= 0.f || track.GetHeight() <= 0.f)
			{
				continue;
			}

			DrawThemeStyleBox(m_themeCache.track, track);
			Ref<ThemeStyleBox> const style =
				m_dragAxis == axis ? m_themeCache.grabber_pressed
								   : (m_hoverAxis == axis ? m_themeCache.grabber_hover : m_themeCache.grabber);
			DrawThemeStyleBox(style, GetGrabberRect(axis));
		}
		break;
	}
}

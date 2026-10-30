#include "MingEngine/Scene/GUI/ScrollContainer.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#include <algorithm>

ScrollContainer::ScrollContainer()
{
	SetClipContents(true);
	SetMouseFilter(MouseFilter::Stop);
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
	Control*      content    = GetContent();
	Vector2 const minimum    = content != nullptr ? content->GetCombinedMinimumSize() : Vector2::Zero;
	bool          horizontal = false;
	bool          vertical   = false;
	for (int i = 0; i < 3; ++i)
	{
		m_pageSize = Vector2(
			std::max(0.f, GetSize().x - (vertical ? 14.f : 0.f)),
			std::max(0.f, GetSize().y - (horizontal ? 14.f : 0.f)));
		horizontal = minimum.x > m_pageSize.x;
		vertical   = minimum.y > m_pageSize.y;
	}

	m_pageSize = Vector2(
		std::max(0.f, GetSize().x - (vertical ? 14.f : 0.f)),
		std::max(0.f, GetSize().y - (horizontal ? 14.f : 0.f)));
	m_contentSize      = Vector2(std::max(minimum.x, m_pageSize.x), std::max(minimum.y, m_pageSize.y));
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
	m_scroll = std::max(0.f, offset);
	Arrange();
}

void ScrollContainer::SetHorizontalScroll(float offset)
{
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
	switch (event.m_type)
	{
	case InputEventType::MouseWheel:
	{
		if ((event.m_modifiers & GLFW_MOD_SHIFT) != 0)
		{
			SetHorizontalScroll(m_horizontalScroll - event.m_scroll.y * 40.f);
		}
		else
		{
			m_horizontalScroll -= event.m_scroll.x * 40.f;
			SetScroll(m_scroll - event.m_scroll.y * 40.f);
		}

		return true;
	}
	case InputEventType::MouseButton:
	{
		if (event.m_mouseButton == 0)
		{
			if (!event.m_pressed)
			{
				m_dragAxis = -1;
				return true;
			}

			if (event.m_position.x >= m_pageSize.x && GetMaximumScroll() > 0.f)
			{
				m_dragAxis = 1;
			}
			else if (event.m_position.y >= m_pageSize.y && m_contentSize.x > m_pageSize.x)
			{
				m_dragAxis = 0;
			}
			else
			{
				return false;
			}

			float const length   = m_dragAxis == 0 ? m_pageSize.x : m_pageSize.y;
			float const extent   = m_dragAxis == 0 ? m_contentSize.x : m_contentSize.y;
			float const scroll   = m_dragAxis == 0 ? m_horizontalScroll : m_scroll;
			float const position = m_dragAxis == 0 ? event.m_position.x : event.m_position.y;
			float const thumb    = std::min(length, std::max(20.f, length * length / std::max(1.f, extent)));
			float const top      = scroll / std::max(1.f, extent - length) * (length - thumb);
			m_dragOffset         = position >= top && position <= top + thumb ? position - top : thumb * 0.5f;
		}
	}
		[[fallthrough]];
	case InputEventType::MouseMotion:
	{
		if (m_dragAxis < 0)
		{
			break;
		}

		float const length   = m_dragAxis == 0 ? m_pageSize.x : m_pageSize.y;
		float const extent   = m_dragAxis == 0 ? m_contentSize.x : m_contentSize.y;
		float const position = m_dragAxis == 0 ? event.m_position.x : event.m_position.y;
		float const thumb    = std::min(length, std::max(20.f, length * length / std::max(1.f, extent)));
		float const offset   = (position - m_dragOffset) / std::max(1.f, length - thumb) * (extent - length);
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
	if (notification == Notification_UpdateLayout)
	{
		Arrange();
	}
	else if (notification == Notification_GuiInputCancelled)
	{
		m_dragAxis = -1;
	}
	else if (notification == Notification_Draw)
	{
		DrawRect(GetLocalRect(), Color(32, 38, 48));
		for (int axis = 0; axis < 2; ++axis)
		{
			float const length = axis == 0 ? m_pageSize.x : m_pageSize.y;
			float const extent = axis == 0 ? m_contentSize.x : m_contentSize.y;
			float const offset = axis == 0 ? m_horizontalScroll : m_scroll;
			if (extent <= length || length <= 0.f)
			{
				continue;
			}

			float const   thumb    = std::min(length, std::max(20.f, length * length / extent));
			float const   top      = offset / (extent - length) * (length - thumb);
			Vector2 const trackMin = axis == 0 ? Vector2(0.f, m_pageSize.y) : Vector2(m_pageSize.x, 0.f);
			Vector2 const trackMax = axis == 0 ? Vector2(length, GetSize().y) : Vector2(GetSize().x, length);
			Vector2 const thumbMin = axis == 0 ? Vector2(top, m_pageSize.y + 2.f) : Vector2(m_pageSize.x + 2.f, top);
			Vector2 const thumbMax =
				axis == 0 ? Vector2(top + thumb, GetSize().y - 2.f) : Vector2(GetSize().x - 2.f, top + thumb);
			DrawRect(AABB2(trackMin, trackMax), Color(28, 34, 45));
			DrawRect(AABB2(thumbMin, thumbMax), Color(110, 125, 145));
		}
	}
}

#include "MingEngine/Scene/GUI/SplitContainer.hpp"

#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float SplitWidth = 8.f;
}

SplitContainer::SplitContainer()
{
	SetClipContents(true);
	SetMouseFilter(MouseFilter::Stop);
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
	float const   gap = first != nullptr && second != nullptr ? SplitWidth : 0.f;
	return m_vertical ? Vector2(std::max(a.x, b.x), a.y + b.y + gap) : Vector2(a.x + b.x + gap, std::max(a.y, b.y));
}

void SplitContainer::SetVertical(bool vertical)
{
	if (m_vertical != vertical)
	{
		m_vertical = vertical;
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

		if (event.m_pressed && first != nullptr && second != nullptr && cursor >= m_offset
			&& cursor < m_offset + SplitWidth)
		{
			m_dragging   = true;
			m_grabOffset = cursor - m_offset;
			return true;
		}

		if (!event.m_pressed && m_dragging)
		{
			m_dragging    = false;
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
		if (m_dragging)
		{
			float const length = std::max(1.f, (m_vertical ? GetSize().y : GetSize().x) - SplitWidth);
			SetSplitRatio((cursor - m_grabOffset) / length);
			return true;
		}
		break;
	}

	return false;
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
		float const   length  = std::max(0.f, (m_vertical ? GetSize().y : GetSize().x) - SplitWidth);
		float const   minimum = m_vertical ? a.y : a.x;
		float const   maximum = std::max(minimum, length - (m_vertical ? b.y : b.x));
		m_offset              = std::clamp(length * m_ratio, minimum, maximum);
		first->SetPosition(Vector2::Zero);
		first->UpdateSize(m_vertical ? Vector2(GetSize().x, m_offset) : Vector2(m_offset, GetSize().y));
		second->SetPosition(m_vertical ? Vector2(0.f, m_offset + SplitWidth) : Vector2(m_offset + SplitWidth, 0.f));
		second->UpdateSize(
			m_vertical ? Vector2(GetSize().x, length - m_offset) : Vector2(length - m_offset, GetSize().y));

		QueueRedraw();

		break;
	}
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		m_dragging = false;
		break;
	case Notification_Draw:
	{
		Control* first;
		Control* second;
		GetPanels(first, second);
		if (first != nullptr && second != nullptr)
		{
			Vector2 const start = m_vertical ? Vector2(0.f, m_offset) : Vector2(m_offset, 0.f);
			Vector2 const end =
				m_vertical ? Vector2(GetSize().x, m_offset + SplitWidth) : Vector2(m_offset + SplitWidth, GetSize().y);
			DrawRect(AABB2(start, end), m_dragging ? Color(85, 147, 192) : Color(44, 53, 67));
		}
		break;
	}
	}
}

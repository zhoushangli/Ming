#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include <algorithm>

void MarginContainer::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Constant, MarginContainer, margin_left);
	BIND_THEME_ITEM(Theme::DataType::Constant, MarginContainer, margin_top);
	BIND_THEME_ITEM(Theme::DataType::Constant, MarginContainer, margin_right);
	BIND_THEME_ITEM(Theme::DataType::Constant, MarginContainer, margin_bottom);
}

float MarginContainer::GetMarginLeft() const
{
	return m_marginOverrides[0].value_or(static_cast<float>(std::max(0, m_themeCache.margin_left)));
}

float MarginContainer::GetMarginTop() const
{
	return m_marginOverrides[1].value_or(static_cast<float>(std::max(0, m_themeCache.margin_top)));
}

float MarginContainer::GetMarginRight() const
{
	return m_marginOverrides[2].value_or(static_cast<float>(std::max(0, m_themeCache.margin_right)));
}

float MarginContainer::GetMarginBottom() const
{
	return m_marginOverrides[3].value_or(static_cast<float>(std::max(0, m_themeCache.margin_bottom)));
}


Vector2 MarginContainer::GetInternalMinimumSize() const
{
	return Container::GetInternalMinimumSize() + Vector2(GetMarginLeft() + GetMarginRight(), GetMarginTop() + GetMarginBottom());
}

void MarginContainer::SetMargins(float left, float top, float right, float bottom)
{
	float const margins[] = { left, top, right, bottom };
	bool changed = false;
	for (int side = 0; side < 4; ++side)
	{
		float const value = std::max(0.f, margins[side]);
		changed = changed || m_marginOverrides[side] != value;
		m_marginOverrides[side] = value;
	}

	if (changed)
	{
		PropagateMinimumSizeChanged();
	}
}

void MarginContainer::SetMargin(int side, float margin)
{
	margin = std::max(0.f, margin);
	if (m_marginOverrides[side] == margin)
	{
		return;
	}

	m_marginOverrides[side] = margin;
	PropagateMinimumSizeChanged();
}

void MarginContainer::RemoveMarginsOverride()
{
	bool changed = false;
	for (auto& margin : m_marginOverrides)
	{
		changed = changed || margin.has_value();
		margin.reset();
	}

	if (changed)
	{
		PropagateMinimumSizeChanged();
	}
}

void MarginContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_UpdateLayout:
	{
		Rect2 rect;
		rect.SetPosition(Vector2(GetMarginLeft(), GetMarginTop()));
		rect.SetSize(GetSize() - Vector2(GetMarginLeft() + GetMarginRight(), GetMarginTop() + GetMarginBottom()));

		for (Node* child : GetChildren())
		{
			if (Control* control = GetLayoutChild(child))
			{
				UpdateChildRect(control, rect);
			}
		}

		break;
	}
	}
}

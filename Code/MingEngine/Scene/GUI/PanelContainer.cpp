#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

void PanelContainer::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::StyleBox, PanelContainer, panel);
}

Vector2 PanelContainer::GetInternalMinimumSize() const
{
	return Container::GetInternalMinimumSize()
		+ (m_themeCache.panel.IsValid() ? m_themeCache.panel->GetMinimumSize() : Vector2::Zero);
}

void PanelContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
		DrawThemeStyleBox(m_themeCache.panel, GetLocalRect());
		break;
	case Notification_UpdateLayout:
	{
		AABB2 const content = m_themeCache.panel.IsValid()
			? m_themeCache.panel->GetContentRect(GetLocalRect()) : GetLocalRect();
		Rect2 rect;
		rect.m_position = content.m_mins;
		rect.m_size = content.GetDimensions();
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

#include "MingEngine/Scene/3D/VisualInstance3D.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"

void VisualInstance3D::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->RegisterVisualizeInstance(this);
		}
		break;
	case Notification_ExitTree:
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->UnregisterVisualizeInstance(this);
		}
		break;
	}
}

void VisualInstance3D::SetVisible(bool visible) { m_isVisible = visible; }
bool VisualInstance3D::GetVisible() const { return m_isVisible; }

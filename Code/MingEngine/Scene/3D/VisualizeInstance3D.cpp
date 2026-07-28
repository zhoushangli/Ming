#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"

void VisualizeInstance3D::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->RegisterVisualizeInstance(this);
		}
		break;
	case NotificationType::ExitTree:
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->UnregisterVisualizeInstance(this);
		}
		break;
	}
}

void VisualizeInstance3D::SetVisible(bool visible) { m_isVisible = visible; }
bool VisualizeInstance3D::GetVisible() const { return m_isVisible; }

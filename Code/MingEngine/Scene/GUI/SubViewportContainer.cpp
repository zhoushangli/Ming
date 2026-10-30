#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include <algorithm>

SubViewportContainer::SubViewportContainer()
{
	SetClipContents(true);
	SetFocusMode(FocusMode::All);
	auto* viewport = MemNew<SubViewport>();
	viewport->SetSerializable(false);
	m_viewportID = viewport->GetObjectID();
	AddNode(viewport);
}

SubViewport* SubViewportContainer::GetSubViewport() const
{
	return ObjectDatabase::GetInstance<SubViewport>(m_viewportID);
}

Vector2 SubViewportContainer::ToViewportPosition(Vector2 windowPosition) const
{
	Vector2 const local      = windowPosition - GetGlobalPosition();
	SubViewport*  viewport   = GetSubViewport();
	Vector2 const resolution = viewport != nullptr ? Vector2(viewport->GetOutputResolution()) : GetSize();
	return Vector2(
		local.x * resolution.x / std::max(1.f, GetSize().x),
		local.y * resolution.y / std::max(1.f, GetSize().y));
}

bool SubViewportContainer::GuiInput(InputEvent const& event)
{
	return m_input ? m_input(event) : true;
}

void SubViewportContainer::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_GuiInputCancelled:
	{
		if (m_input)
		{
			InputEvent event;
			event.m_type = InputEventType::WindowFocus;
			m_input(event);
		}

		break;
	}

	case Notification_UpdateLayout:
	case Notification_Resized:
	{
		if (SubViewport* viewport = GetSubViewport(); viewport != nullptr && GetSize().x >= 1.f && GetSize().y >= 1.f)
		{
			viewport->UpdateResolution(IntVec2(static_cast<int>(GetSize().x), static_cast<int>(GetSize().y)));
		}

		QueueRedraw();
		break;
	}

	case Notification_Draw:
	{
		if (SubViewport* viewport = GetSubViewport())
		{
			Rect2 rect;
			rect.SetSize(GetSize());
			DrawViewportRect(viewport->GetViewportRID(), rect);
		}

		break;
	}
	}
}

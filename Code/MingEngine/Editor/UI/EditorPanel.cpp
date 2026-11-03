#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include <algorithm>

void EditorPanel::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::StyleBox, EditorPanel, panel);
}

EditorPanel::EditorPanel(char const* title, bool isOpen) : m_title(title), m_isOpen(isOpen)
{
	SetName(title);
	SetSerializable(false);
	SetClipContents(true);
	SetVisible(isOpen);
}

void EditorPanel::Initialize(EditorUIContext const& context)
{
	m_context = context;
	if (!m_initialized)
	{
		m_initialized = true;
		OnInitialize();
	}
}

void EditorPanel::Open()
{
	UIData data;
	Open(data);
}

void EditorPanel::Open(UIData const& data)
{
	m_isOpen = true;
	SetVisible(true);
	OnOpen(data);
}

void EditorPanel::Close()
{
	if (m_isOpen)
	{
		m_isOpen = false;
		if (GetViewport() != nullptr)
		{
			GetViewport()->CancelGuiInteraction(this);
		}

		OnClose();
		SetVisible(false);
	}
}

void EditorPanel::Render(EditorUIContext& context)
{
	Initialize(context);
	if (m_isOpen)
	{
		OnRender(context);
	}
}

Vector2 EditorPanel::GetInternalMinimumSize() const
{
	Vector2 minimum;
	for (Node* child : GetChildren())
	{
		if (auto* control = dynamic_cast<Control*>(child);
			control != nullptr && control->IsVisible() && !control->IsQueuedForDeletion())
		{
			Vector2 const size = control->GetCombinedMinimumSize();
			minimum.x          = std::max(minimum.x, size.x);
			minimum.y          = std::max(minimum.y, size.y);
		}
	}

	return minimum;
}

void EditorPanel::OnOpen([[maybe_unused]] UIData const& data)
{
}

void EditorPanel::OnClose()
{
}

void EditorPanel::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
		DrawThemeStyleBox(m_themeCache.panel, GetLocalRect());
		break;
	case Notification_Resized:
		for (Node* child : GetChildren())
		{
			if (auto* control = dynamic_cast<Control*>(child))
			{
				control->SetPosition(Vector2::Zero);
				control->UpdateSize(GetSize());
			}
		}
		break;
	}
}

// Scene/Core/Window.cpp
#include "MingEngine/Scene/Core/Window.hpp"
#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

Window::Window()
{
	IntVec2 size = g_engine->m_windowSystem->GetClientDimensions();
	if (size.x > 0 && size.y > 0)
	{
		SetResolution(size);
	}
}

void Window::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	{
		g_engine->m_renderServer->ViewportSetPresentToScreen(m_viewportRID, true);
		m_windowResizedEventID = RegisterEvent("WindowResized", this, &Window::OnWindowResized);

		ObjectID const windowID = GetObjectID();
		g_engine->m_inputSystem->SetInputEventCallback(
			[windowID](InputEvent const& event)
			{
				if (Window* window = ObjectDatabase::GetInstance<Window>(windowID))
				{
					window->OnInputEvent(event);
				}
			});
		break;
	}

	case Notification_ExitTree:
	{
		g_engine->m_renderServer->ViewportSetPresentToScreen(m_viewportRID, false);
		UnregisterEvent("WindowResized", m_windowResizedEventID);
		m_windowResizedEventID = 0;

		g_engine->m_inputSystem->SetInputEventCallback({});
		break;
	}
	}
}

void Window::OnInputEvent(InputEvent const& event)
{
	if (EditorNode* editor = EditorNode::Get(); editor != nullptr && editor->HandleGuiInput(event))
	{
		return;
	}

	ProcessInputEvent(event);
}

bool Window::OnWindowResized(EventArgs& args)
{
	int width  = args.GetValue("width", 0);
	int height = args.GetValue("height", 0);

	UpdateResolution(IntVec2(width, height));

	return true;
}

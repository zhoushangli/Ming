// Scene/Core/Window.cpp
#include "MingEngine/Scene/Core/Window.hpp"
#include "MingEngine/Core/Object/Callable.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
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

void Window::BindMethods()
{
	ClassDatabase::BindMethod("OnWindowResized", &Window::OnWindowResized, { "width", "height" });
}

void Window::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
	{
		g_engine->m_renderServer->ViewportSetPresentToScreen(m_viewportRID, true);
		g_engine->m_windowSystem->Connect("framebuffer_resized", Callable(this, "OnWindowResized"));

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
		g_engine->m_windowSystem->Disconnect("framebuffer_resized", Callable(this, "OnWindowResized"));

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

void Window::OnWindowResized(int width, int height)
{
	UpdateResolution(IntVec2(width, height));
}

// Scene/Core/Window.cpp
#include "MingEngine/Scene/Core/Window.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

Window::Window() { SyncWindowSize(); }

void Window::SyncWindowSize()
{
	IntVec2 size = g_engine->m_windowSystem->GetClientDimensions();
	if (size.x > 0 && size.y > 0)
	{
		SetResolution(size);
	}
}

void Window::OnNotification(int notification)
{
	if (notification == Notification_EnterTree)
	{
		g_engine->m_renderServer->ViewportSetPresentToScreen(m_viewportRID, true);
	}
	else if (notification == Notification_ExitTree)
	{
		g_engine->m_renderServer->ViewportSetPresentToScreen(m_viewportRID, false);
	}
}
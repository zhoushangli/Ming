#include "MingEngine/Scene/Core/CanvasLayer.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

CanvasLayer::CanvasLayer() { m_canvasLayerRID = g_engine->m_renderServer->CanvasLayerCreate(); }

CanvasLayer::~CanvasLayer() { g_engine->m_renderServer->CanvasLayerFree(m_canvasLayerRID); }

void CanvasLayer::OnNotification(int notification)
{
	RenderServer* server = g_engine->m_renderServer;
	switch (notification)
	{
	case Notification_EnterTree:
		server->CanvasLayerSetViewport(m_canvasLayerRID, m_data.m_viewport->GetViewportRID());
		break;
	case Notification_ExitTree:
		server->CanvasLayerSetViewport(m_canvasLayerRID, RID::Invalid);
		break;
	}
}

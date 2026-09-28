#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

CanvasItem::CanvasItem() { m_canvasItemRID = g_engine->m_renderServer->CanvasItemCreate(); }

CanvasItem::~CanvasItem() { g_engine->m_renderServer->CanvasItemFree(m_canvasItemRID); }

CanvasItem* CanvasItem::GetParentItem() const { return dynamic_cast<CanvasItem*>(GetParent()); }

Vector2 CanvasItem::GetLocalPosition() const { return Vector2::Zero; }

Vector2 CanvasItem::GetGlobalPosition() const
{
	CanvasItem* parent = GetParentItem();

	if (parent != nullptr)
	{
		return parent->GetGlobalPosition() + GetLocalPosition();
	}

	return GetLocalPosition();
}

void CanvasItem::SetVisible(bool visible)
{
	m_visible = visible;
	g_engine->m_renderServer->CanvasItemSetVisible(m_canvasItemRID, visible);
}

bool CanvasItem::IsVisible() const { return m_visible; }

bool CanvasItem::IsVisibleInHierarchy() const
{
	if (!m_visible)
	{
		return false;
	}

	CanvasItem* parent = GetParentItem();
	return parent == nullptr || parent->IsVisibleInHierarchy();
}

void CanvasItem::QueueRedraw()
{
	m_redrawPending = true;
	if (GetSceneTree() == nullptr)
	{
		return;
	}

	RenderServer* server = g_engine->m_renderServer;
	server->CanvasItemClear(m_canvasItemRID);
	Notification(Notification_Draw);
	m_redrawPending = false;
}

void CanvasItem::SyncPosition()
{
	g_engine->m_renderServer->CanvasItemSetPosition(m_canvasItemRID, GetLocalPosition());
}

void CanvasItem::OnNotification(int notification)
{
	RenderServer* server = g_engine->m_renderServer;
	switch (notification)
	{
	case Notification_EnterTree:
		server->CanvasItemSetViewport(m_canvasItemRID, m_data.m_viewport->GetViewportRID());
		SyncPosition();
		server->CanvasItemSetVisible(m_canvasItemRID, m_visible);
		if (m_redrawPending)
		{
			QueueRedraw();
		}
		break;
	case Notification_ExitTree:
		server->CanvasItemSetViewport(m_canvasItemRID, RID::Invalid);
		break;
	}
}

void CanvasItem::DrawRect(AABB2 const& rect, Color const& color)
{
	if (rect.GetWidth() <= 0.0f || rect.GetHeight() <= 0.0f)
	{
		return;
	}

	g_engine->m_renderServer->CanvasItemAddRect(m_canvasItemRID, rect, color);
}

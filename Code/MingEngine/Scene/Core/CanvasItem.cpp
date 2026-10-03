#include "MingEngine/Scene/Core/CanvasItem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/Core/CanvasLayer.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

CanvasItem::CanvasItem() { m_canvasItemRID = g_engine->m_renderServer->CanvasItemCreate(); }

CanvasItem::~CanvasItem() { g_engine->m_renderServer->CanvasItemFree(m_canvasItemRID); }

CanvasItem* CanvasItem::GetParentItem() const
{
	for (Node* ancestor = GetParent(); ancestor != nullptr && ancestor != m_data.m_viewport;
		 ancestor       = ancestor->GetParent())
	{
		if (CanvasItem* item = dynamic_cast<CanvasItem*>(ancestor))
		{
			return item;
		}
		if (dynamic_cast<CanvasLayer*>(ancestor) != nullptr)
		{
			break;
		}
	}
	return nullptr;
}

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
	{
		CanvasItem* parentItem = nullptr;
		m_canvasLayerRID       = RID::Invalid;
		for (Node* ancestor = GetParent(); ancestor != nullptr && ancestor != m_data.m_viewport;
			 ancestor       = ancestor->GetParent())
		{
			if (CanvasItem* item = dynamic_cast<CanvasItem*>(ancestor))
			{
				parentItem       = item;
				m_canvasLayerRID = parentItem->m_canvasLayerRID;
				break;
			}
			if (CanvasLayer* layer = dynamic_cast<CanvasLayer*>(ancestor))
			{
				m_canvasLayerRID = layer->GetCanvasLayerRID();
				break;
			}
		}
		if (!m_canvasLayerRID.IsValid())
		{
			m_canvasLayerRID = m_data.m_viewport->GetCanvasLayerRID();
		}

		server->CanvasItemSetViewport(m_canvasItemRID, m_data.m_viewport->GetViewportRID());
		if (parentItem != nullptr)
		{
			server->CanvasItemSetParent(m_canvasItemRID, parentItem->m_canvasItemRID);
		}
		else
		{
			server->CanvasLayerAddChild(m_canvasLayerRID, m_canvasItemRID);
		}
		server->CanvasItemSetPosition(m_canvasItemRID, GetLocalPosition());
		server->CanvasItemSetVisible(m_canvasItemRID, m_visible);

		if (m_redrawPending)
		{
			QueueRedraw();
		}
		break;
	}
	case Notification_ExitTree:
		server->CanvasItemSetParent(m_canvasItemRID, RID::Invalid);
		server->CanvasItemSetViewport(m_canvasItemRID, RID::Invalid);

		if (m_canvasLayerRID.IsValid())
		{
			m_canvasLayerRID = RID::Invalid;
			server->CanvasLayerRemoveChild(m_canvasLayerRID, m_canvasItemRID);
		}

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

void CanvasItem::DrawTextureRect(Ref<TextureResource> const& texture, Rect2 const& rect, bool isTiling)
{
	if (!texture.IsValid())
	{
		return;
	}

	g_engine->m_renderServer->CanvasItemAddTextureRect(m_canvasItemRID, texture, rect, isTiling);
}

void CanvasItem::DrawTextureRectRegion(
	Ref<TextureResource> const& texture, Rect2 const& rect, Rect2 const& sourceRect, Color const& color)
{
	if (!texture.IsValid())
	{
		return;
	}

	g_engine->m_renderServer->CanvasItemAddTextureRectRegion(m_canvasItemRID, texture, rect, sourceRect, color);
}
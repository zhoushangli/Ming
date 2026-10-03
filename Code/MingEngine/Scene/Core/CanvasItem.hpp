#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/Rect2.hpp"
#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

class RenderServer;

class CanvasItem : public Node
{
	MCLASS(CanvasItem, Node);

public:
	enum
	{
		Notification_Draw = 30
	};

	CanvasItem();
	~CanvasItem() override;

	CanvasItem*     GetParentItem() const;
	virtual Vector2 GetLocalPosition() const;
	Vector2         GetGlobalPosition() const;

	void SetVisible(bool visible);
	bool IsVisible() const;
	bool IsVisibleInHierarchy() const;

	void QueueRedraw();

protected:
	static void BindMethods() {}

	// ATTENTION!
	// All the rect here are in local space, not global space.
	// We do this because we a CanvasItem drawing
	// he just need to care about what he looks like in his own local space, not the global space.
	void DrawRect(AABB2 const& rect, Color const& color);
	void DrawTextureRect(Ref<TextureResource> const& texture, Rect2 const& rect, bool isTiling = false);
	void DrawTextureRectRegion(
		Ref<TextureResource> const& texture,
		Rect2 const&                rect,
		Rect2 const&                sourceRect,
		Color const&                color = Color::White);

	void SyncPosition();

	void OnNotification(int notification);

private:
	bool m_visible       = true;
	bool m_redrawPending = true;

	RID m_canvasItemRID  = RID::Invalid;
	RID m_canvasLayerRID = RID::Invalid;
};

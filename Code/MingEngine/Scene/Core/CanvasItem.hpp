#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
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

	static void BindMethods() {}
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
	void DrawRect(AABB2 const& rect, Color const& color);
	void SyncPosition();
	void OnNotification(int notification);

private:
	bool m_visible       = true;
	bool m_redrawPending = true;

	RID m_canvasItemRID = RID::Invalid;
};

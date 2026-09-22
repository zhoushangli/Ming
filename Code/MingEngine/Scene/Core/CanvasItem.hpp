#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include <memory>
#include <vector>

class RenderServer;

class CanvasItem : public Node
{
	MCLASS(CanvasItem, Node);

public:
	enum
	{
		Notification_Draw = 50
	};

	static void BindMethods() {}

	CanvasItem*     GetParentItem() const;
	virtual Vector2 GetLocalPosition() const;
	Vector2         GetGlobalPosition() const;

	void SetVisible(bool visible);
	bool IsVisible() const;
	bool IsVisibleInHierarchy() const;

	void QueueRedraw();

	RenderRequest BuildRenderRequest(RenderServer& renderer, float viewportHeight);

protected:
	void DrawRect(AABB2 const& rect, Color const& color);

private:
	bool m_visible       = true;
	bool m_redrawPending = true;

	std::vector<Vertex>           m_localVertices;
	std::unique_ptr<VertexBuffer> m_vertexBuffer;
};
#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"

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

void CanvasItem::SetVisible(bool visible) { m_visible = visible; }

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

void CanvasItem::QueueRedraw() { m_redrawPending = true; }

void CanvasItem::DrawRect(AABB2 const& rect, Color const& color)
{
	if (rect.GetWidth() <= 0.0f || rect.GetHeight() <= 0.0f)
	{
		return;
	}

	Vector3 a(rect.m_mins.x, rect.m_mins.y, 0.0f);
	Vector3 b(rect.m_maxs.x, rect.m_mins.y, 0.0f);
	Vector3 c(rect.m_maxs.x, rect.m_maxs.y, 0.0f);
	Vector3 d(rect.m_mins.x, rect.m_maxs.y, 0.0f);

	m_localVertices.emplace_back(a, color, Vector2::Zero);
	m_localVertices.emplace_back(b, color, Vector2::Zero);
	m_localVertices.emplace_back(c, color, Vector2::Zero);

	m_localVertices.emplace_back(a, color, Vector2::Zero);
	m_localVertices.emplace_back(c, color, Vector2::Zero);
	m_localVertices.emplace_back(d, color, Vector2::Zero);
}

RenderRequest CanvasItem::BuildRenderRequest(RenderServer& renderer, float viewportHeight)
{
	RenderRequest request;

	if (!IsVisibleInHierarchy())
	{
		return request;
	}

	// 1) Rebuild local geometry when drawing content changes
	if (m_redrawPending)
	{
		m_redrawPending = false;
		m_localVertices.clear();
		Notification(Notification_Draw);
	}

	if (m_localVertices.empty())
	{
		return request;
	}

	// 2) Convert local UI coordinates to render coordinates
	Vector2             position = GetGlobalPosition();
	std::vector<Vertex> vertices = m_localVertices;

	for (Vertex& vertex : vertices)
	{
		vertex.m_position.x += position.x;
		vertex.m_position.y = viewportHeight - (position.y + vertex.m_position.y);
	}

	// 3) Upload geometry into a buffer owned by this CanvasItem
	unsigned int byteSize = static_cast<unsigned int>(vertices.size() * sizeof(Vertex));

	if (!m_vertexBuffer || m_vertexBuffer->GetSize() != byteSize)
	{
		m_vertexBuffer.reset(renderer.CreateVertexBuffer(byteSize, sizeof(Vertex)));
	}

	renderer.CopyCPUToGPU(vertices.data(), byteSize, m_vertexBuffer.get());

	// 4) Submit an unlit, alpha-blended UI request
	request.m_pass           = RenderRequestPass::UI;
	request.m_vertexBuffer   = m_vertexBuffer.get();
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::READ_ONLY_ALWAYS;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;

	return request;
}
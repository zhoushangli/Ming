#include "MingEngine/Editor/GizmosShapes.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Math/AABB3.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Engine/Render/VertexUtils.hpp"

namespace
{
Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);
}

EditorGizmoVisual3D::~EditorGizmoVisual3D()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
}

RenderRequest EditorGizmoVisual3D::SubmitRenderRequest() const
{
	RenderRequest request;
	request.m_pass           = RenderRequestPass::Opaque;
	request.m_modelToWorld   = GetWorldTransform();
	request.m_vertexBuffer   = m_vertexBuffer;
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	return request;
}

EditorWorldGrid3D::EditorWorldGrid3D()
{
	constexpr int   kHalfExtent     = 50;
	constexpr float kLineHeight     = 0.01f;
	constexpr float kBaseThickness  = 0.002f;
	constexpr float kAxisThickness  = 0.01f;
	constexpr float kFadeStart      = 10.f;
	constexpr float kFadeEnd        = 30.f;

	auto AddGridStrip = [this](AABB3 const& bounds, Rgba8 color)
	{
		Vec3 const  center   = bounds.GetCenter();
		float const distance = sqrtf(center.x * center.x + center.y * center.y);
		color.a = static_cast<unsigned char>(RangeMapClamped(distance, kFadeStart, kFadeEnd, 255.f, 0.f));
		AddVertsForAABB3D(m_vertices, bounds, color);
	};

	for (int lineIndex = -kHalfExtent; lineIndex <= kHalfExtent; ++lineIndex)
	{
		bool const  isAxis      = lineIndex == 0;
		float const thickness  = isAxis ? kAxisThickness : kBaseThickness;
		float const halfWidth  = thickness * 0.5f;
		float const lineOffset = static_cast<float>(lineIndex);
		Rgba8 xLineColor       = isAxis ? kAxisXColor : Rgba8::Gray;
		Rgba8 yLineColor       = isAxis ? kAxisYColor : Rgba8::Gray;

		for (int segmentIndex = -kHalfExtent; segmentIndex < kHalfExtent; ++segmentIndex)
		{
			float const segmentStart = static_cast<float>(segmentIndex);
			float const segmentEnd   = segmentStart + 1.f;

			AddGridStrip(
				AABB3(Vec3(segmentStart, lineOffset - halfWidth, 0.f),
					Vec3(segmentEnd, lineOffset + halfWidth, kLineHeight)),
				xLineColor);
			AddGridStrip(
				AABB3(Vec3(lineOffset - halfWidth, segmentStart, 0.f),
					Vec3(lineOffset + halfWidth, segmentEnd, kLineHeight)),
				yLineColor);
		}
	}

	if (!m_vertices.empty() && g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(m_vertices);
	}
}

EditorWorldAxis3D::EditorWorldAxis3D()
{
	constexpr float kAxisLength = 1.f;
	constexpr float kAxisRadius = 0.03f;

	AddVertsForArrow3D(m_vertices, Vec3::Zero, Vec3(kAxisLength, 0.f, 0.f), kAxisRadius, kAxisXColor);
	AddVertsForArrow3D(m_vertices, Vec3::Zero, Vec3(0.f, kAxisLength, 0.f), kAxisRadius, kAxisYColor);
	AddVertsForArrow3D(m_vertices, Vec3::Zero, Vec3(0.f, 0.f, kAxisLength), kAxisRadius, kAxisZColor);

	if (!m_vertices.empty() && g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(m_vertices);
	}
}

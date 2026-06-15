#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"

#include "GizmosShapes.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"

namespace
{
Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);
} // namespace

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
	constexpr int kHalfExtent      = 1000;
	constexpr float kLineThickness = 0.005f;
	constexpr float kHalfThickness = kLineThickness * 0.5f;

	for (int lineIndex = -kHalfExtent; lineIndex <= kHalfExtent; ++lineIndex)
	{
		Vec3 aCol = Vec3(-(float)kHalfExtent, (float)lineIndex, 0.f);
		Vec3 bCol = Vec3((float)kHalfExtent, (float)lineIndex, 0.f);
		Vec3 aRow = Vec3((float)lineIndex, -(float)kHalfExtent, 0.f);
		Vec3 bRow = Vec3((float)lineIndex, (float)kHalfExtent, 0.f);

		Vec3 thickness = Vec3(kHalfThickness, kHalfThickness, kHalfThickness);

		AddVertsForAABB3D(m_verts, AABB3(aCol - thickness, bCol + thickness), Rgba8::Gray);
		AddVertsForAABB3D(m_verts, AABB3(aRow - thickness, bRow + thickness), Rgba8::Gray);
	}

	if (!m_verts.empty() && g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(m_verts);
	}
}

RenderRequest EditorWorldGrid3D::SubmitRenderRequest() const
{
	RenderRequest request;
	request.m_pass           = RenderRequestPass::Opaque;
	request.m_modelToWorld   = GetWorldTransform();
	request.m_vertexBuffer   = m_vertexBuffer;
	request.m_shader         = g_engine->m_renderer->CreateOrGetShader("Data/Shaders/GizmosGrid");
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::READ_ONLY_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	return request;
}

EditorWorldAxis3D::EditorWorldAxis3D()
{
	constexpr float kAxisExtent    = 1000.f;
	constexpr float kAxisThickness = 0.01f;
	constexpr float kHalfThickness = kAxisThickness * 0.5f;

	AddVertsForAABB3D(
		m_verts,
		AABB3(Vec3(-kAxisExtent, -kHalfThickness, -kHalfThickness), Vec3(kAxisExtent, kHalfThickness, kHalfThickness)),
		kAxisXColor);
	AddVertsForAABB3D(
		m_verts,
		AABB3(Vec3(-kHalfThickness, -kAxisExtent, -kHalfThickness), Vec3(kHalfThickness, kAxisExtent, kHalfThickness)),
		kAxisYColor);
	AddVertsForAABB3D(
		m_verts,
		AABB3(Vec3(-kHalfThickness, -kHalfThickness, -kAxisExtent), Vec3(kHalfThickness, kHalfThickness, kAxisExtent)),
		kAxisZColor);

	if (!m_verts.empty() && g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(m_verts);
	}
}


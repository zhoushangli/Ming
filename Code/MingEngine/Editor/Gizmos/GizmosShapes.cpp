#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"

#include "GizmosShapes.hpp"
#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

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
	constexpr int   kHalfExtent    = 1000;
	constexpr float kLineThickness = 0.01f;
	constexpr float kHalfThickness = kLineThickness * 0.5f;

	for (int lineIndex = -kHalfExtent; lineIndex <= kHalfExtent; ++lineIndex)
	{
		if (lineIndex == 0)
		{
			continue; // Skip the center line, which is drawn by EditorWorldAxis3D
		}

		Vec3 aCol = Vec3(-(float)kHalfExtent, (float)lineIndex, 0.f);
		Vec3 bCol = Vec3((float)kHalfExtent, (float)lineIndex, 0.f);
		Vec3 aRow = Vec3((float)lineIndex, -(float)kHalfExtent, 0.f);
		Vec3 bRow = Vec3((float)lineIndex, (float)kHalfExtent, 0.f);

		Vec3 thickness = Vec3(kHalfThickness, kHalfThickness, kHalfThickness);

		AddVertsForAABB3D(m_verts, AABB3(aCol - thickness, bCol + thickness), Color::Gray);
		AddVertsForAABB3D(m_verts, AABB3(aRow - thickness, bRow + thickness), Color::Gray);
	}

	if (!m_verts.empty() && g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(
			m_verts.data(),
			(unsigned int)(m_verts.size() * sizeof(Vertex)),
			sizeof(Vertex));
	}
}

RenderRequest EditorWorldGrid3D::SubmitRenderRequest() const
{
	Ref<ShaderResource> shaderResource =
		g_engine->m_renderer->GetBuiltinShaderResource("GizmosGrid", BuiltinShaders::GizmosGrid);

	RenderRequest request;
	request.m_pass           = RenderRequestPass::Opaque;
	request.m_modelToWorld   = GetWorldTransform();
	request.m_vertexBuffer   = m_vertexBuffer;
	request.m_shader         = shaderResource.IsValid() ? shaderResource->GetShader() : nullptr;
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::READ_ONLY_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;

	return request;
}

EditorWorldAxis3D::EditorWorldAxis3D(Vec3 const& axisStart, Vec3 const& axisEnd, Color const& color)
{
	// 1) Build the shader-expanded line quad
	AddVertsForQuad3D(
		m_verts,
		Vec3(0.f, -0.5f, 0.f),
		Vec3(0.f, 0.5f, 0.f),
		Vec3(0.f, 0.5f, 1.f),
		Vec3(0.f, -0.5f, 1.f),
		color);

	// 2) Store the axis start in the model matrix translation
	SetWorldPosition(axisStart);

	// 3) Encode the axis end in the model matrix scale for GizmosAxis.hlsl
	// Encodes the axis end in the model matrix diagonal for GizmosAxis.hlsl
	// e.g. SetWorldScale(Vec3(1000.f, 0.f, 0.f))
	SetWorldScale(axisEnd);

	// 4) Create the static vertex buffer
	if (!m_verts.empty() && g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(
			m_verts.data(),
			(unsigned int)(m_verts.size() * sizeof(Vertex)),
			sizeof(Vertex));
	}
}

RenderRequest EditorWorldAxis3D::SubmitRenderRequest() const
{
	RenderRequest request;
	request.m_pass         = RenderRequestPass::Opaque;
	request.m_modelToWorld = GetWorldTransform();
	request.m_vertexBuffer = m_vertexBuffer;
	Ref<ShaderResource> shaderResource =
		g_engine->m_renderer->GetBuiltinShaderResource("GizmosAxis", BuiltinShaders::GizmosAxis);
	request.m_shader         = shaderResource.IsValid() ? shaderResource->GetShader() : nullptr;
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	return request;
}

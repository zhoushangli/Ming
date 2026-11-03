#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"

#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include <cstring>

void EditorGizmoVisual3D::UploadGeometry(
	char const* shaderName, std::string_view shaderSource, RenderRequestPass pass, DepthMode depth)
{
	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server == nullptr || m_verts.empty())
	{
		return;
	}

	if (!m_mesh.IsValid())
	{
		m_mesh                     = CreateRef<MeshResource>();
		m_mesh->m_materialResource = CreateRef<MaterialResource>();
	}

	auto& material             = m_mesh->m_materialResource;
	material->m_shaderResource = server->GetBuiltinShaderResource(shaderName, shaderSource);
	material->m_depthMode      = depth;
	material->m_blendMode      = BlendMode::ALPHA;
	material->m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	m_mesh->m_vertexStride     = sizeof(Vertex);
	m_mesh->m_vertexCount      = static_cast<uint32_t>(m_verts.size());
	m_mesh->m_vertices.resize(m_verts.size() * sizeof(Vertex));
	std::memcpy(m_mesh->m_vertices.data(), m_verts.data(), m_mesh->m_vertices.size());
	std::vector<unsigned int> indices = m_indices;
	if (indices.empty())
	{
		indices.resize(m_verts.size());
		for (size_t i = 0; i < indices.size(); ++i)
		{
			indices[i] = static_cast<unsigned int>(i);
		}
	}

	m_mesh->m_indexCount = static_cast<uint32_t>(indices.size());
	m_mesh->m_indices.resize(indices.size() * sizeof(unsigned int));
	std::memcpy(m_mesh->m_indices.data(), indices.data(), m_mesh->m_indices.size());
	m_mesh->InitGPUResources();
	server->InstanceSetBase(m_instanceRID, m_mesh->GetMeshRID());
	server->InstanceSetRenderPass(m_instanceRID, pass);
}

void EditorGizmoVisual3D::SetRenderPriority(int priority)
{
	g_engine->m_renderServer->InstanceSetRenderPriority(m_instanceRID, priority);
}

void EditorGizmoVisual3D::SetGizmoColor(Color const& color)
{
	if (m_mesh.IsValid())
	{
		m_mesh->m_materialResource->m_tint = color;
	}
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

		Vector3 aCol = Vector3(-(float)kHalfExtent, (float)lineIndex, 0.f);
		Vector3 bCol = Vector3((float)kHalfExtent, (float)lineIndex, 0.f);
		Vector3 aRow = Vector3((float)lineIndex, -(float)kHalfExtent, 0.f);
		Vector3 bRow = Vector3((float)lineIndex, (float)kHalfExtent, 0.f);

		Vector3 thickness = Vector3(kHalfThickness, kHalfThickness, kHalfThickness);

		AddVertsForAABB3D(m_verts, AABB3(aCol - thickness, bCol + thickness), Color::Gray);
		AddVertsForAABB3D(m_verts, AABB3(aRow - thickness, bRow + thickness), Color::Gray);
	}

	UploadGeometry(
		"GizmosGrid",
		BuiltinShaders::GizmosGrid,
		RenderRequestPass::Opaque,
		DepthMode::READ_WRITE_LESS_EQUAL);
}

EditorWorldAxis3D::EditorWorldAxis3D(Vector3 const& axisStart, Vector3 const& axisEnd, Color const& color)
{
	// 1) Build the shader-expanded line quad
	AddVertsForQuad3D(
		m_verts,
		Vector3(0.f, -0.5f, 0.f),
		Vector3(0.f, 0.5f, 0.f),
		Vector3(0.f, 0.5f, 1.f),
		Vector3(0.f, -0.5f, 1.f),
		color);

	// 2) Store the axis start in the model matrix translation
	SetWorldPosition(axisStart);

	// 3) Encode the axis end in the model matrix scale for GizmosAxis.hlsl
	// Encodes the axis end in the model matrix diagonal for GizmosAxis.hlsl
	// e.g. SetWorldScale(Vec3(1000.f, 0.f, 0.f))
	SetWorldScale(axisEnd);

	UploadGeometry(
		"GizmosAxis",
		BuiltinShaders::GizmosAxis,
		RenderRequestPass::Opaque,
		DepthMode::READ_WRITE_LESS_EQUAL);
}

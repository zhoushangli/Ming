#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

MeshResource::~MeshResource()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;
}

bool MeshResource::IsEmpty() const
{
	return m_vertices.empty() || m_indices.empty() || m_vertexCount == 0 || m_indexCount == 0;
}

bool MeshResource::CopyFrom(Resource const& other)
{
	// 1) Validate type
	MeshResource const* otherMesh = dynamic_cast<MeshResource const*>(&other);
	if (otherMesh == nullptr)
	{
		return false;
	}

	// 2) Destroy old GPU buffers
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	// 3) Copy CPU data fields
	m_vertexFormat = otherMesh->m_vertexFormat;
	m_vertexStride = otherMesh->m_vertexStride;
	m_vertexCount  = otherMesh->m_vertexCount;
	m_vertices     = otherMesh->m_vertices;

	m_indexFormat = otherMesh->m_indexFormat;
	m_indexStride = otherMesh->m_indexStride;
	m_indexCount  = otherMesh->m_indexCount;
	m_indices     = otherMesh->m_indices;

	m_textures = otherMesh->m_textures;

	// 4) Recreate GPU buffers from copied CPU data
	if (g_engine != nullptr && g_engine->m_renderer != nullptr && !IsEmpty())
	{
		m_vertexBuffer =
			g_engine->m_renderer->CreateVertexBuffer(m_vertices.data(), m_vertexCount * m_vertexStride, m_vertexStride);
		m_indexBuffer =
			g_engine->m_renderer->CreateIndexBuffer(m_indices.data(), m_indexCount * m_indexStride, m_indexStride);
	}

	return true;
}

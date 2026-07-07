#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

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

	// 2) Copy CPU data fields
	m_vertexFormat = otherMesh->m_vertexFormat;
	m_vertexStride = otherMesh->m_vertexStride;
	m_vertexCount  = otherMesh->m_vertexCount;
	m_vertices     = otherMesh->m_vertices;

	m_indexFormat = otherMesh->m_indexFormat;
	m_indexStride = otherMesh->m_indexStride;
	m_indexCount  = otherMesh->m_indexCount;
	m_indices     = otherMesh->m_indices;

	m_textureResources = otherMesh->m_textureResources;

	// 3) Recreate GPU buffers from copied CPU data
	InitGPUResources();

	return true;
}

void MeshResource::InitGPUResources()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	m_vertexBuffer =
		g_engine->m_renderer->CreateVertexBuffer(m_vertices.data(), m_vertexCount * m_vertexStride, m_vertexStride);
	m_indexBuffer =
		g_engine->m_renderer->CreateIndexBuffer(m_indices.data(), m_indexCount * m_indexStride, m_indexStride);

	// Build triangle list for raycast
	m_triangles.clear();
	if (m_vertexFormat == "Vertex" && m_vertexCount > 0 && m_indexCount >= 3)
	{
		Vertex const*   vertexData = reinterpret_cast<Vertex const*>(m_vertices.data());
		uint32_t const* indexData  = reinterpret_cast<uint32_t const*>(m_indices.data());

		for (uint32_t i = 0; i + 2 < m_indexCount; i += 3)
		{
			uint32_t indexA = indexData[i + 0];
			uint32_t indexB = indexData[i + 1];
			uint32_t indexC = indexData[i + 2];

			if (indexA >= m_vertexCount || indexB >= m_vertexCount || indexC >= m_vertexCount)
			{
				continue;
			}

			Vec3 pointA = vertexData[indexA].m_position;
			Vec3 pointB = vertexData[indexB].m_position;
			Vec3 pointC = vertexData[indexC].m_position;

			Triangle3 triangle(pointA, pointB, pointC);
			m_triangles.push_back(triangle);
		}
	}
}

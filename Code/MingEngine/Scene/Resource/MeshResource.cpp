#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

#include <utility>

MeshResource::~MeshResource()
{
	// The registration is released while the RenderServer still owns the GPU buffers.
	RenderServer* server = (g_engine != nullptr) ? g_engine->m_renderServer : nullptr;
	if (server != nullptr && m_meshRID.IsValid())
	{
		server->MeshFree(m_meshRID);
		m_meshRID = RID::Invalid;
	}
}

bool MeshResource::IsEmpty() const
{
	return m_vertices.empty() || m_indices.empty() || m_vertexCount == 0 || m_indexCount == 0;
}

bool MeshResource::CopyFrom(Resource&& other)
{
	// 1) Validate type
	MeshResource* otherMesh = dynamic_cast<MeshResource*>(&other);
	if (otherMesh == nullptr)
	{
		return false;
	}

	// 2) Move CPU data fields
	MoveBaseFrom(std::move(other));
	m_vertexFormat = std::move(otherMesh->m_vertexFormat);
	m_vertexStride = otherMesh->m_vertexStride;
	m_vertexCount  = otherMesh->m_vertexCount;
	m_vertices     = std::move(otherMesh->m_vertices);

	m_indexFormat = std::move(otherMesh->m_indexFormat);
	m_indexStride = otherMesh->m_indexStride;
	m_indexCount  = otherMesh->m_indexCount;
	m_indices     = std::move(otherMesh->m_indices);

	m_materialResource = std::move(otherMesh->m_materialResource);
	m_bounds           = otherMesh->m_bounds;
	m_triangles        = std::move(otherMesh->m_triangles);

	// 3) Hand the loaded GPU data to the registration of this resource.
	//    The other resource gives up its RID so it cannot free what this one keeps.
	RenderServer* server = (g_engine != nullptr) ? g_engine->m_renderServer : nullptr;
	if (server != nullptr)
	{
		if (m_meshRID.IsValid())
		{
			server->MeshRefresh(m_meshRID, *this);
		}
		else
		{
			m_meshRID = server->MeshCreate(*this);
		}

		if (otherMesh->m_meshRID.IsValid())
		{
			server->MeshFree(otherMesh->m_meshRID);
			otherMesh->m_meshRID = RID::Invalid;
		}
	}

	return true;
}

void MeshResource::InitGPUResources()
{
	// 1) Build the CPU triangle list used by raycasts.
	// 2) Register the mesh once and then keep that RID stable across reloads.
	m_triangles.clear();
	Vertex const*   vertexData = reinterpret_cast<Vertex const*>(m_vertices.data());
	uint32_t const* indexData  = reinterpret_cast<uint32_t const*>(m_indices.data());

	m_bounds = GetVertexBounds3D(vertexData, m_vertexCount);

	for (uint32_t i = 0; i + 2 < m_indexCount; i += 3)
	{
		uint32_t indexA = indexData[i + 0];
		uint32_t indexB = indexData[i + 1];
		uint32_t indexC = indexData[i + 2];

		if (indexA >= m_vertexCount || indexB >= m_vertexCount || indexC >= m_vertexCount)
		{
			continue;
		}

		Vector3 pointA = vertexData[indexA].m_position;
		Vector3 pointB = vertexData[indexB].m_position;
		Vector3 pointC = vertexData[indexC].m_position;

		Triangle3 triangle(pointA, pointB, pointC);
		m_triangles.push_back(triangle);
	}

	RenderServer* server = (g_engine != nullptr) ? g_engine->m_renderServer : nullptr;
	if (server == nullptr || IsEmpty())
	{
		return;
	}

	if (m_meshRID.IsValid())
	{
		server->MeshRefresh(m_meshRID, *this);
		return;
	}

	m_meshRID = server->MeshCreate(*this);
}

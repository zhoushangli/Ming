#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Engine/Render/IndexBuffer.hpp"
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

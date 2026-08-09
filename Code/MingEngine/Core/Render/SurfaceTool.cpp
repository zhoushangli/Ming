#include "MingEngine/Core/Render/SurfaceTool.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <cmath>
#include <unordered_map>

namespace
{
Vector3 GetArbitraryTangent(Vector3 const& normal)
{
	if (normal.GetLengthSquared() <= 0.f)
	{
		return Vector3::Forward;
	}

	Vector3 const normalizedNormal = normal.GetNormalized();
	Vector3 const helperAxis       = std::abs(normalizedNormal.z) < 0.999f ? Vector3::Up : Vector3::Right;
	return Vector3::CrossProduct(helperAxis, normalizedNormal).GetNormalized();
}
} // namespace

void SurfaceTool::SetColor(Color const& color) { m_prevColor = color; }

void SurfaceTool::SetNormal(Vector3 const& normal) { m_prevNormal = normal; }

void SurfaceTool::SetTangent(Vector3 const& tangent) { m_prevTangent = tangent; }

void SurfaceTool::SetBitangent(Vector3 const& bitangent) { m_prevBitangent = bitangent; }

void SurfaceTool::SetUV(Vector2 const& uv) { m_prevUV = uv; }

void SurfaceTool::SetSmoothingGroup(uint32_t smoothingGroup) { m_prevSmoothingGroup = smoothingGroup; }

void SurfaceTool::AddVertex(Vector3 const& position)
{
	SurfaceVertex vertex;
	vertex.m_vertex.m_position  = position;
	vertex.m_vertex.m_color     = m_prevColor;
	vertex.m_vertex.m_uv        = m_prevUV;
	vertex.m_vertex.m_tangent   = m_prevTangent;
	vertex.m_vertex.m_bitangent = m_prevBitangent;
	vertex.m_vertex.m_normal    = m_prevNormal;
	vertex.m_smoothingGroup     = m_prevSmoothingGroup;
	m_vertices.push_back(vertex);
}

void SurfaceTool::AddIndex(uint32_t index) { m_indices.push_back(index); }

void SurfaceTool::Index()
{
	if (!m_indices.empty())
	{
		return;
	}

	std::vector<SurfaceVertex>                                     indexedVertices;
	std::unordered_map<SurfaceVertex, uint32_t, SurfaceVertexHash> vertexMap;
	indexedVertices.reserve(m_vertices.size());
	m_indices.reserve(m_vertices.size());

	for (SurfaceVertex const& vertex : m_vertices)
	{
		auto const iter = vertexMap.find(vertex);
		if (iter != vertexMap.end())
		{
			m_indices.push_back(iter->second);
			continue;
		}

		uint32_t const index = static_cast<uint32_t>(indexedVertices.size());
		indexedVertices.push_back(vertex);
		vertexMap.emplace(vertex, index);
		m_indices.push_back(index);
	}

	m_vertices.swap(indexedVertices);
}

void SurfaceTool::Deindex()
{
	if (m_indices.empty())
	{
		return;
	}

	std::vector<SurfaceVertex> unindexedVertices;
	unindexedVertices.reserve(m_indices.size());
	for (uint32_t index : m_indices)
	{
		GUARANTEE_OR_DIE(index < m_vertices.size(), "SurfaceTool index is outside the vertex array.");
		unindexedVertices.push_back(m_vertices[index]);
	}

	m_vertices.swap(unindexedVertices);
	m_indices.clear();
}

void SurfaceTool::GenerateNormals(bool flip)
{
	bool const hadIndices = !m_indices.empty();
	if (hadIndices)
	{
		Deindex();
	}

	GUARANTEE_OR_DIE(m_vertices.size() % 3 == 0, "Vertex count must be a multiple of 3 to generate normals.");

	std::unordered_map<SmoothNormalKey, Vector3, SmoothNormalKeyHash> smoothNormals;
	for (size_t triangleStart = 0; triangleStart < m_vertices.size(); triangleStart += 3)
	{
		Vector3 const& position0 = m_vertices[triangleStart + 0].m_vertex.m_position;
		Vector3 const& position1 = m_vertices[triangleStart + 1].m_vertex.m_position;
		Vector3 const& position2 = m_vertices[triangleStart + 2].m_vertex.m_position;

		Vector3 normal = Vector3::CrossProduct(position1 - position0, position2 - position0);
		if (normal.GetLengthSquared() > 0.f)
		{
			normal.Normalize();
		}
		if (flip)
		{
			normal = -normal;
		}

		for (size_t corner = 0; corner < 3; ++corner)
		{
			SurfaceVertex& vertex = m_vertices[triangleStart + corner];
			if (vertex.m_smoothingGroup == NoSmoothingGroup)
			{
				vertex.m_vertex.m_normal = normal;
				continue;
			}

			smoothNormals[SmoothNormalKey(vertex)] += normal;
		}
	}

	for (SurfaceVertex& vertex : m_vertices)
	{
		if (vertex.m_smoothingGroup == NoSmoothingGroup)
		{
			continue;
		}

		auto const iter = smoothNormals.find(SmoothNormalKey(vertex));
		if (iter == smoothNormals.end() || iter->second.GetLengthSquared() <= 0.f)
		{
			vertex.m_vertex.m_normal = Vector3::Zero;
			continue;
		}

		vertex.m_vertex.m_normal = iter->second.GetNormalized();
	}

	if (hadIndices)
	{
		Index();
	}
}

void SurfaceTool::GenerateTangents()
{
	bool const hadIndices = !m_indices.empty();
	if (hadIndices)
	{
		Deindex();
	}

	GUARANTEE_OR_DIE(m_vertices.size() % 3 == 0, "Vertex count must be a multiple of 3 to generate tangents.");

	for (size_t triangleStart = 0; triangleStart < m_vertices.size(); triangleStart += 3)
	{
		SurfaceVertex& surfaceVertex0 = m_vertices[triangleStart + 0];
		SurfaceVertex& surfaceVertex1 = m_vertices[triangleStart + 1];
		SurfaceVertex& surfaceVertex2 = m_vertices[triangleStart + 2];
		Vertex&        vertex0        = surfaceVertex0.m_vertex;
		Vertex&        vertex1        = surfaceVertex1.m_vertex;
		Vertex&        vertex2        = surfaceVertex2.m_vertex;

		Vector3 const edge1       = vertex1.m_position - vertex0.m_position;
		Vector3 const edge2       = vertex2.m_position - vertex0.m_position;
		Vector2 const deltaUV1    = vertex1.m_uv - vertex0.m_uv;
		Vector2 const deltaUV2    = vertex2.m_uv - vertex0.m_uv;
		float const   determinant = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;

		Vector3 tangent = Vector3::Zero;
		if (std::abs(determinant) > 0.000001f)
		{
			tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) / determinant;
			if (tangent.GetLengthSquared() > 0.f)
			{
				tangent.Normalize();
			}
		}

		Vertex* const triangleVertices[3] = { &vertex0, &vertex1, &vertex2 };
		for (Vertex* vertex : triangleVertices)
		{
			vertex->m_tangent   = tangent.GetLengthSquared() > 0.f ? tangent : GetArbitraryTangent(vertex->m_normal);
			vertex->m_bitangent = Vector3::CrossProduct(vertex->m_normal, vertex->m_tangent);
			if (vertex->m_bitangent.GetLengthSquared() > 0.f)
			{
				vertex->m_bitangent.Normalize();
			}
		}
	}

	if (hadIndices)
	{
		Index();
	}
}

void SurfaceTool::Clear()
{
	m_vertices.clear();
	m_indices.clear();

	m_prevColor          = Color::White;
	m_prevNormal         = Vector3::Zero;
	m_prevTangent        = Vector3::Zero;
	m_prevBitangent      = Vector3::Zero;
	m_prevUV             = Vector2::Zero;
	m_prevSmoothingGroup = NoSmoothingGroup;
}

Ref<MeshResource> SurfaceTool::CreateMeshResource() const
{
	std::vector<Vertex> vertices;
	vertices.reserve(m_vertices.size());
	for (SurfaceVertex const& surfaceVertex : m_vertices)
	{
		vertices.push_back(surfaceVertex.m_vertex);
	}

	Ref<MeshResource> meshResource = CreateRef<MeshResource>();
	meshResource->m_vertexFormat   = "PCUTBN";
	meshResource->m_vertexStride   = sizeof(Vertex);
	meshResource->m_vertexCount    = static_cast<uint32_t>(vertices.size());
	meshResource->m_vertices.assign(
		reinterpret_cast<uint8_t const*>(vertices.data()),
		reinterpret_cast<uint8_t const*>(vertices.data()) + vertices.size() * sizeof(Vertex));

	meshResource->m_indexFormat = "uint32";
	meshResource->m_indexStride = sizeof(uint32_t);
	meshResource->m_indexCount  = static_cast<uint32_t>(m_indices.size());
	meshResource->m_indices.assign(
		reinterpret_cast<uint8_t const*>(m_indices.data()),
		reinterpret_cast<uint8_t const*>(m_indices.data()) + m_indices.size() * sizeof(uint32_t));

	meshResource->InitGPUResources();
	return meshResource;
}

#pragma once

#include "MingEngine/Core/Object/Resource.hpp"

#include <cstdint>
#include <string>
#include <vector>

class IndexBuffer;
class VertexBuffer;

struct MeshTextureData
{
	std::string          m_name;
	std::string          m_format;
	uint32_t             m_width    = 0;
	uint32_t             m_height   = 0;
	uint32_t             m_channels = 0;
	std::vector<uint8_t> m_data;
};

class MeshResource : public Resource
{
	MCLASS(MeshResource, Resource)

public:
	MeshResource() = default;
	MeshResource(MeshResource const& copy)            = delete;
	MeshResource& operator=(MeshResource const& copy) = delete;
	~MeshResource();

	bool IsEmpty() const;

public:
	std::string          m_vertexFormat;
	uint32_t             m_vertexStride = 0;
	uint32_t             m_vertexCount  = 0;
	std::vector<uint8_t> m_vertices;

	std::string          m_indexFormat = "uint32";
	uint32_t             m_indexStride = 4;
	uint32_t             m_indexCount  = 0;
	std::vector<uint8_t> m_indices;

	std::vector<MeshTextureData> m_textures;

	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer*  m_indexBuffer  = nullptr;
};

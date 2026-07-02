#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/Object/Resource.hpp"

#include <string>
#include <vector>

class IndexBuffer;
class Texture;
class VertexBuffer;

struct ImportTextureData
{
	std::string          m_name;
	std::string          m_format;
	uint32_t             m_width    = 0;
	uint32_t             m_height   = 0;
	uint32_t             m_channels = 0;
	std::vector<uint8_t> m_data;
};

struct ImportMeshData
{
	std::string m_name;

	std::string          m_vertexFormat;
	uint32_t             m_vertexStride = 0;
	uint32_t             m_vertexCount  = 0;
	std::vector<uint8_t> m_vertices;

	std::string          m_indexFormat = "uint32";
	uint32_t             m_indexStride = 4;
	uint32_t             m_indexCount  = 0;
	std::vector<uint8_t> m_indices;

	std::vector<ImportTextureData> m_textures;
};

struct MeshData
{
	std::vector<Vertex>       m_vertices;
	std::vector<unsigned int> m_indices;
	Texture*                  m_texture = nullptr;

	void Clear();
	bool IsEmpty() const;
};

class MeshResource : public Resource
{
	MCLASS(MeshResource, Resource)

public:
	static MeshResource* CreateOrGetMesh(std::string const& modelFilePath, float scale = 1.f);

	MeshResource(MeshResource const& copy)            = delete;
	MeshResource& operator=(MeshResource const& copy) = delete;
	~MeshResource();

	bool IsEmpty() const;

	static void ClearLoadedMeshes();

public:
	std::string   m_modelFilePath;
	float         m_scale = 1.f;
	MeshData      m_meshData;
	AABB3         m_localBounds;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer*  m_indexBuffer  = nullptr;

private:
	MeshResource(std::string const& modelFilePath, float scale);

private:
	static std::vector<MeshResource*> s_loadedMeshes;
};

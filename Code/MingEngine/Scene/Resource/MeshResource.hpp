#pragma once

#include "MingEngine/Engine/Render/Vertex.hpp"
#include "MingEngine/Engine/Math/AABB3.hpp"

#include <string>
#include <vector>

class IndexBuffer;
class Texture;
class VertexBuffer;

struct MeshData
{
	std::vector<Vertex>       m_vertices;
	std::vector<unsigned int> m_indices;
	Texture*                  m_texture = nullptr;

	void Clear();
	bool IsEmpty() const;
};

class MeshResource
{
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

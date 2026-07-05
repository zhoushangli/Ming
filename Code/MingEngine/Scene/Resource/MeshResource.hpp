#pragma once

#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <cstdint>
#include <string>
#include <vector>

class GPUTexture;
class IndexBuffer;
class VertexBuffer;

class MeshResource : public Resource
{
	MCLASS(MeshResource, Resource)

public:
	MeshResource()                                    = default;
	MeshResource(MeshResource const& copy)            = delete;
	MeshResource& operator=(MeshResource const& copy) = delete;
	~MeshResource();

	bool IsEmpty() const;
	bool CopyFrom(Resource const& other) override;

	void InitGPUResources();

protected:
	static void BindMethods() {}

public:
	std::string          m_vertexFormat;
	uint32_t             m_vertexStride = 0;
	uint32_t             m_vertexCount  = 0;
	std::vector<uint8_t> m_vertices;

	std::string          m_indexFormat = "uint32";
	uint32_t             m_indexStride = 4;
	uint32_t             m_indexCount  = 0;
	std::vector<uint8_t> m_indices;

	// Texture references: paths for serialization, Refs for runtime, GPU handles for rendering
	std::vector<std::string>          m_texturePaths;
	std::vector<Ref<TextureResource>> m_textureResources;

	// GPU side data handles
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer*  m_indexBuffer  = nullptr;
};

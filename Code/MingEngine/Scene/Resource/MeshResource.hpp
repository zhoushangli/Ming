#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/Triangle3.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Scene/Resource/MaterialResource.hpp"

#include <cstdint>
#include <string>
#include <vector>

class MeshResource : public Resource
{
	MCLASS(MeshResource, Resource)

public:
	MeshResource()                                    = default;
	MeshResource(MeshResource const& copy)            = delete;
	MeshResource& operator=(MeshResource const& copy) = delete;
	~MeshResource();

	bool IsEmpty() const;
	bool CopyFrom(Resource&& other) override;

	void InitGPUResources();

	RID GetMeshRID() const { return m_meshRID; }

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

	// Material this mesh draws with; an invalid reference makes the RenderServer use its default material.
	// e.g. the OBJ importer stores the MTL textures here and InitGPUResources() hands it to the MeshData
	Ref<MaterialResource> m_materialResource;

	// Mesh RID on the RenderServer; RID::Invalid means the mesh has no GPU data yet.
	// e.g. InitGPUResources() registers it and MeshInstance3D binds this RID as its Base
	RID m_meshRID = RID::Invalid;

	AABB3                  m_bounds; // Mainly for raycast
	std::vector<Triangle3> m_triangles;
};

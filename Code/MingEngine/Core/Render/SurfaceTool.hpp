#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include <cstdint>
#include <functional>
#include <vector>

class SurfaceTool : public RefCounted
{
public:
	static constexpr uint32_t NoSmoothingGroup = UINT32_MAX;

private:
	struct SurfaceVertex
	{
		Vertex   m_vertex;
		uint32_t m_smoothingGroup = NoSmoothingGroup;

		bool operator==(SurfaceVertex const& other) const
		{
			return m_vertex.m_position == other.m_vertex.m_position && m_vertex.m_color == other.m_vertex.m_color
				   && m_vertex.m_uv == other.m_vertex.m_uv && m_vertex.m_tangent == other.m_vertex.m_tangent
				   && m_vertex.m_bitangent == other.m_vertex.m_bitangent && m_vertex.m_normal == other.m_vertex.m_normal
				   && m_smoothingGroup == other.m_smoothingGroup;
		}
	};

	struct SmoothNormalKey
	{
		Vec3     m_position;
		uint32_t m_smoothingGroup;

		SmoothNormalKey(SurfaceVertex const& vertex)
			: m_position(vertex.m_vertex.m_position), m_smoothingGroup(vertex.m_smoothingGroup)
		{
		}

		bool operator==(SmoothNormalKey const& other) const
		{
			return m_position == other.m_position && m_smoothingGroup == other.m_smoothingGroup;
		}
	};

	struct HashHelper
	{
		static void Combine(size_t& seed, size_t value) noexcept
		{
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		static void CombineVec2(size_t& seed, Vec2 const& value) noexcept
		{
			Combine(seed, std::hash<float>{}(value.x));
			Combine(seed, std::hash<float>{}(value.y));
		}

		static void CombineVec3(size_t& seed, Vec3 const& value) noexcept
		{
			Combine(seed, std::hash<float>{}(value.x));
			Combine(seed, std::hash<float>{}(value.y));
			Combine(seed, std::hash<float>{}(value.z));
		}
	};

	struct SurfaceVertexHash
	{
		size_t operator()(SurfaceVertex const& surfaceVertex) const noexcept
		{
			Vertex const& vertex = surfaceVertex.m_vertex;
			size_t        hash   = 0;
			HashHelper::CombineVec3(hash, vertex.m_position);
			HashHelper::Combine(hash, std::hash<unsigned char>{}(vertex.m_color.r));
			HashHelper::Combine(hash, std::hash<unsigned char>{}(vertex.m_color.g));
			HashHelper::Combine(hash, std::hash<unsigned char>{}(vertex.m_color.b));
			HashHelper::Combine(hash, std::hash<unsigned char>{}(vertex.m_color.a));
			HashHelper::CombineVec2(hash, vertex.m_uv);
			HashHelper::CombineVec3(hash, vertex.m_tangent);
			HashHelper::CombineVec3(hash, vertex.m_bitangent);
			HashHelper::CombineVec3(hash, vertex.m_normal);
			HashHelper::Combine(hash, std::hash<uint32_t>{}(surfaceVertex.m_smoothingGroup));
			return hash;
		}
	};

	struct SmoothNormalKeyHash
	{
		size_t operator()(SmoothNormalKey const& key) const noexcept
		{
			size_t hash = 0;
			HashHelper::CombineVec3(hash, key.m_position);
			HashHelper::Combine(hash, std::hash<uint32_t>{}(key.m_smoothingGroup));
			return hash;
		}
	};

public:
	void SetColor(Color const& color);
	void SetNormal(Vec3 const& normal);
	void SetTangent(Vec3 const& tangent);
	void SetBitangent(Vec3 const& bitangent);
	void SetUV(Vec2 const& uv);
	void SetSmoothingGroup(uint32_t smoothingGroup);

	void AddVertex(Vec3 const& position);
	void AddIndex(uint32_t index);

	// Indexed will clean up the index and rebuild it
	void Index();
	// Unindexed will clean up the index and expand the vertex list to be unindexed
	void Deindex();
	void GenerateNormals(bool flip = false);
	void GenerateTangents();

	void Clear();

	Ref<MeshResource> CreateMeshResource() const;

private:
	std::vector<SurfaceVertex> m_vertices;
	std::vector<uint32_t>      m_indices;

	// memory
	Color    m_prevColor          = Color::White;
	Vec3     m_prevNormal         = Vec3::Zero;
	Vec3     m_prevTangent        = Vec3::Zero;
	Vec3     m_prevBitangent      = Vec3::Zero;
	Vec2     m_prevUV             = Vec2::Zero;
	uint32_t m_prevSmoothingGroup = NoSmoothingGroup;
};

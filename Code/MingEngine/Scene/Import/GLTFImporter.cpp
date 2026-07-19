#include "MingEngine/Scene/Import/GLTFImporter.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Render/SurfaceTool.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

// tiny_fltf_v3 use fopen and strcpy, which are considered unsafe by MSVC.
// Disable the warnings for this file.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#include "ThirdParty/tinygltf/tiny_gltf_v3.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <limits>

namespace
{

struct AccessorView
{
	tg3_accessor const* m_accessor = nullptr;
	uint8_t const*      m_data     = nullptr;
	uint32_t            m_stride   = 0;
};

// clang-format off
Matrix4x4 GLTFImportMatrix = Matrix4x4(
	1.f, 0.f, 0.f, 0.f,
	0.f, 0.f, -1.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 1.f
);

std::vector<ImportOptions> const kGLTFImportOptions = {
	{ PropertyInfo(Variant::Type::Vec3, "Scale Mesh", 		 PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Default), Variant(Vec3::One) },
};
// clang-format on

bool TryGetAccessorView(tg3_model const& model, int32_t accessorIndex, AccessorView& outView)
{
	outView = AccessorView();

	if (accessorIndex < 0 || static_cast<uint32_t>(accessorIndex) >= model.accessors_count)
	{
		return false;
	}

	tg3_accessor const& accessor = model.accessors[accessorIndex];
	if (accessor.sparse.is_sparse || accessor.count == 0 || accessor.buffer_view < 0
		|| static_cast<uint32_t>(accessor.buffer_view) >= model.buffer_views_count)
	{
		return false;
	}

	tg3_buffer_view const& bufferView = model.buffer_views[accessor.buffer_view];
	if (bufferView.buffer < 0 || static_cast<uint32_t>(bufferView.buffer) >= model.buffers_count)
	{
		return false;
	}

	tg3_buffer const& buffer = model.buffers[bufferView.buffer];
	if (buffer.data.data == nullptr || bufferView.byte_offset > buffer.data.count
		|| bufferView.byte_length > buffer.data.count - bufferView.byte_offset)
	{
		return false;
	}

	int32_t const componentSize  = tg3_component_size(accessor.component_type);
	int32_t const componentCount = tg3_num_components(accessor.type);
	int32_t const stride         = tg3_accessor_byte_stride(&accessor, &bufferView);
	if (componentSize <= 0 || componentCount <= 0 || stride <= 0)
	{
		return false;
	}

	uint64_t const elementSize = static_cast<uint64_t>(componentSize) * static_cast<uint64_t>(componentCount);
	if (static_cast<uint64_t>(stride) < elementSize || accessor.byte_offset > bufferView.byte_length)
	{
		return false;
	}

	uint64_t const elementCountAfterFirst = accessor.count - 1;
	if (elementCountAfterFirst > (std::numeric_limits<uint64_t>::max() - elementSize) / static_cast<uint64_t>(stride))
	{
		return false;
	}

	uint64_t const requiredBytes = elementCountAfterFirst * static_cast<uint64_t>(stride) + elementSize;
	if (requiredBytes > bufferView.byte_length - accessor.byte_offset)
	{
		return false;
	}

	uint64_t const byteOffset = bufferView.byte_offset + accessor.byte_offset;

	outView.m_accessor = &accessor;
	outView.m_data     = buffer.data.data + byteOffset;
	outView.m_stride   = static_cast<uint32_t>(stride);
	return true;
}

int32_t FindAttribute(tg3_primitive const& primitive, char const* attributeName)
{
	for (uint32_t i = 0; i < primitive.attributes_count; ++i)
	{
		tg3_str_int_pair const& attribute = primitive.attributes[i];
		if (tg3_str_equals_cstr(attribute.key, attributeName))
		{
			return attribute.value;
		}
	}

	return -1;
}

float ReadFloat(uint8_t const* data)
{
	float value = 0.f;
	memcpy(&value, data, sizeof(float));
	return value;
}

uint32_t ReadIndex(uint8_t const* data, int32_t componentType)
{
	switch (componentType)
	{
	case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
		return *data;

	case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
	{
		uint16_t value = 0;
		memcpy(&value, data, sizeof(value));
		return value;
	}

	case TG3_COMPONENT_TYPE_UNSIGNED_INT:
	{
		uint32_t value = 0;
		memcpy(&value, data, sizeof(value));
		return value;
	}

	default:
		return UINT32_MAX;
	}
}

} // namespace

std::vector<std::string> GLTFImporter::GetSupportedExtensions() const { return { ".gltf", ".glb" }; }

std::string GLTFImporter::GetVisibleName() const { return "glTF as Mesh"; }

std::string GLTFImporter::GetImportedExtension() const { return "mesh"; }

std::vector<ImportOptions> const GLTFImporter::GetImportOptions() const { return kGLTFImportOptions; }

Ref<Resource> GLTFImporter::Import(
	std::unordered_map<std::string, Variant> const& importOptions, std::string const& sourceVirtualPath)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return Ref<Resource>();
	}

	// 1) Parse the source file and load its external buffer when present.
	tinygltf3::Model      model;
	tinygltf3::ErrorStack errors;

	tg3_parse_options options;
	tg3_parse_options_init(&options);
	options.images_as_is = 1;

	std::filesystem::path physicalPath;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(sourceVirtualPath, physicalPath))
	{
		return Ref<Resource>();
	}

	std::string const    path   = physicalPath.string();
	tg3_error_code const result = tinygltf3::parse_file(model, errors, path.c_str(), &options);

	if (result != TG3_OK || errors.has_error())
	{
		return Ref<Resource>();
	}

	// 2) Accept one triangle mesh with one primitive for the minimal importer.
	tg3_model const& gltfModel = *model.get();
	if (gltfModel.meshes_count != 1)
	{
		return Ref<Resource>();
	}

	tg3_mesh const& mesh = gltfModel.meshes[0];
	if (mesh.primitives_count != 1)
	{
		return Ref<Resource>();
	}

	tg3_primitive const& primitive = mesh.primitives[0];
	if (primitive.mode != -1 && primitive.mode != TG3_MODE_TRIANGLES)
	{
		return Ref<Resource>();
	}

	// 3) Read the required position attribute.
	int32_t const positionAccessorIndex = FindAttribute(primitive, "POSITION");
	AccessorView  positionView;
	if (!TryGetAccessorView(gltfModel, positionAccessorIndex, positionView)
		|| positionView.m_accessor->component_type != TG3_COMPONENT_TYPE_FLOAT
		|| positionView.m_accessor->type != TG3_TYPE_VEC3
		|| positionView.m_accessor->count > std::numeric_limits<uint32_t>::max())
	{
		return Ref<Resource>();
	}

	uint64_t const      vertexCount = positionView.m_accessor->count;
	std::vector<Vertex> vertices;
	vertices.resize(static_cast<size_t>(vertexCount));
	for (uint64_t i = 0; i < vertexCount; ++i)
	{
		Vertex&        vertex       = vertices[i];
		uint8_t const* positionData = positionView.m_data + i * positionView.m_stride;
		vertex.m_position           = Vec3(
			ReadFloat(positionData + sizeof(float) * 0),
			ReadFloat(positionData + sizeof(float) * 1),
			ReadFloat(positionData + sizeof(float) * 2));
		vertex.m_color = Color::White;
	}

	// 4) Read optional attributes when their supported float formats are present.
	int32_t const normalAccessorIndex = FindAttribute(primitive, "NORMAL");
	AccessorView  normalView;
	if (normalAccessorIndex >= 0)
	{
		if (!TryGetAccessorView(gltfModel, normalAccessorIndex, normalView)
			|| normalView.m_accessor->component_type != TG3_COMPONENT_TYPE_FLOAT
			|| normalView.m_accessor->type != TG3_TYPE_VEC3 || normalView.m_accessor->count != vertexCount)
		{
			return Ref<Resource>();
		}

		for (uint64_t i = 0; i < vertexCount; ++i)
		{
			Vertex&        vertex     = vertices[i];
			uint8_t const* normalData = normalView.m_data + i * normalView.m_stride;
			vertex.m_normal           = Vec3(
				ReadFloat(normalData + sizeof(float) * 0),
				ReadFloat(normalData + sizeof(float) * 1),
				ReadFloat(normalData + sizeof(float) * 2));
		}
	}

	int32_t const uvAccessorIndex = FindAttribute(primitive, "TEXCOORD_0");
	AccessorView  uvView;
	if (uvAccessorIndex >= 0)
	{
		if (!TryGetAccessorView(gltfModel, uvAccessorIndex, uvView)
			|| uvView.m_accessor->component_type != TG3_COMPONENT_TYPE_FLOAT || uvView.m_accessor->type != TG3_TYPE_VEC2
			|| uvView.m_accessor->count != vertexCount)
		{
			return Ref<Resource>();
		}

		for (uint64_t i = 0; i < vertexCount; ++i)
		{
			Vertex&        vertex = vertices[i];
			uint8_t const* uvData = uvView.m_data + i * uvView.m_stride;
			vertex.m_uv           = Vec2(ReadFloat(uvData + sizeof(float) * 0), ReadFloat(uvData + sizeof(float) * 1));
		}
	}

	int32_t const tangentAccessorIndex = FindAttribute(primitive, "TANGENT");
	AccessorView  tangentView;
	if (tangentAccessorIndex >= 0)
	{
		if (!TryGetAccessorView(gltfModel, tangentAccessorIndex, tangentView)
			|| tangentView.m_accessor->component_type != TG3_COMPONENT_TYPE_FLOAT
			|| tangentView.m_accessor->type != TG3_TYPE_VEC4 || tangentView.m_accessor->count != vertexCount)
		{
			return Ref<Resource>();
		}

		for (uint64_t i = 0; i < vertexCount; ++i)
		{
			Vertex&        vertex      = vertices[i];
			uint8_t const* tangentData = tangentView.m_data + i * tangentView.m_stride;
			vertex.m_tangent           = Vec3(
				ReadFloat(tangentData + sizeof(float) * 0),
				ReadFloat(tangentData + sizeof(float) * 1),
				ReadFloat(tangentData + sizeof(float) * 2));
			vertex.m_bitangent =
				Vec3::CrossProduct(vertex.m_normal, vertex.m_tangent) * ReadFloat(tangentData + sizeof(float) * 3);
		}
	}

	// 5) Convert source indices to the uint32 format used by MeshResource.
	int32_t const         indexAccessorIndex = primitive.indices;
	AccessorView          indexView;
	std::vector<uint32_t> indices;
	if (indexAccessorIndex >= 0)
	{
		if (!TryGetAccessorView(gltfModel, indexAccessorIndex, indexView))
		{
			return Ref<Resource>();
		}

		tg3_accessor const& indexAccessor  = *indexView.m_accessor;
		bool const          validIndexType = indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE
											 || indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT
											 || indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_INT;
		if (indexAccessor.type != TG3_TYPE_SCALAR || !validIndexType || indexAccessor.normalized
			|| indexAccessor.count > std::numeric_limits<uint32_t>::max() || indexAccessor.count % 3 != 0)
		{
			return Ref<Resource>();
		}

		indices.reserve(static_cast<size_t>(indexAccessor.count));

		for (uint64_t i = 0; i < indexAccessor.count; ++i)
		{
			uint8_t const* indexData  = indexView.m_data + i * indexView.m_stride;
			uint32_t const localIndex = ReadIndex(indexData, indexAccessor.component_type);
			if (localIndex >= vertices.size())
			{
				return Ref<Resource>();
			}
			indices.push_back(localIndex);
		}
	}
	else
	{
		if (vertexCount % 3 != 0)
		{
			return Ref<Resource>();
		}

		indices.reserve(vertices.size());
		for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); ++i)
		{
			indices.push_back(i);
		}
	}

	// 6) Apply the engine coordinate conversion and the optional import scale.
	Vec3       scaleMesh   = Vec3::One;
	auto const scaleOption = importOptions.find("Scale Mesh");
	if (scaleOption != importOptions.end() && scaleOption->second.GetType() == Variant::Type::Vec3)
	{
		scaleMesh = scaleOption->second.As<Vec3>();
	}

	Matrix4x4 importTransform = GLTFImportMatrix;
	importTransform.AppendScaleNonUniform3D(scaleMesh);
	TransformVertexArray3D(vertices, importTransform);

	// 7) Build the engine mesh resource through SurfaceTool.
	SurfaceTool surfaceTool;
	for (Vertex const& vertex : vertices)
	{
		surfaceTool.SetColor(vertex.m_color);
		surfaceTool.SetUV(vertex.m_uv);
		surfaceTool.SetNormal(vertex.m_normal);
		surfaceTool.SetTangent(vertex.m_tangent);
		surfaceTool.SetBitangent(vertex.m_bitangent);
		surfaceTool.AddVertex(vertex.m_position);
	}
	for (uint32_t index : indices)
	{
		surfaceTool.AddIndex(index);
	}

	Ref<MeshResource> meshData = surfaceTool.CreateMeshResource();
	meshData->SetName(physicalPath.stem().string());

	// 8) Resolve the first base color texture from the first material.
	// Walk: primitive.material → material.baseColorTexture → texture.source → image.uri
	if (primitive.material >= 0 && static_cast<uint32_t>(primitive.material) < gltfModel.materials_count)
	{
		tg3_material const& material = gltfModel.materials[primitive.material];
		int32_t const       texIndex = material.pbr_metallic_roughness.base_color_texture.index;
		if (texIndex >= 0 && static_cast<uint32_t>(texIndex) < gltfModel.textures_count)
		{
			tg3_texture const& texture = gltfModel.textures[texIndex];
			if (texture.source >= 0 && static_cast<uint32_t>(texture.source) < gltfModel.images_count)
			{
				tg3_image const& image = gltfModel.images[texture.source];
				if (image.uri.data != nullptr)
				{
					std::string const     uri(image.uri.data, image.uri.len);
					std::filesystem::path texPhysicalPath = physicalPath.parent_path() / uri;
					std::string const     texVirtualPath  = g_engine->m_fileSystem->ToVirtualPath(texPhysicalPath);

					if (ResourceImporter::EnsureImported(texVirtualPath))
					{
						Ref<Resource> texResource = ResourceLoader::Load(texVirtualPath);
						meshData->m_textureResources.push_back(texResource);
					}
				}
			}
		}
	}

	return meshData;
}

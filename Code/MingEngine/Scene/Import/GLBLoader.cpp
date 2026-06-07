#include "MingEngine/Scene/Import/GLBLoader.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Math/IntVec2.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

#include "ThirdParty/stb/stb_image.h"

// These pragma are use to disable warnings from the tinygltf3 implementation
// There are some old style string functions and some unused static functions
#pragma warning(push)
#pragma warning(disable : 4505)
#pragma warning(disable : 4996)
#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#include "ThirdParty/tinygltf/tiny_gltf_v3.h"
#pragma warning(pop)

#include <climits>
#include <cstdint>
#include <cstring>
#include <string>

namespace
{
std::string ToString(tg3_str const& text)
{
	if (text.data == nullptr || text.len == 0)
	{
		return "";
	}

	return std::string(text.data, text.len);
}

std::string GetDirectoryPath(std::string const& filePath)
{
	size_t const slashIndex = filePath.find_last_of("/\\");
	if (slashIndex == std::string::npos)
	{
		return "";
	}

	return filePath.substr(0, slashIndex + 1);
}

bool IsAbsolutePath(std::string const& filePath)
{
	if (filePath.size() >= 2 && filePath[1] == ':')
	{
		return true;
	}

	return !filePath.empty() && (filePath[0] == '/' || filePath[0] == '\\');
}

std::string JoinPath(std::string const& directory, std::string const& filePath)
{
	if (filePath.empty() || IsAbsolutePath(filePath) || directory.empty())
	{
		return filePath;
	}

	char const lastChar = directory.back();
	if (lastChar == '/' || lastChar == '\\')
	{
		return directory + filePath;
	}

	return directory + "/" + filePath;
}

bool AttributeNameMatches(tg3_str const& attributeName, char const* expectedName)
{
	size_t const expectedLength = strlen(expectedName);
	return attributeName.data != nullptr && attributeName.len == expectedLength
		   && memcmp(attributeName.data, expectedName, expectedLength) == 0;
}

int FindAttributeAccessor(tg3_primitive const& primitive, char const* attributeName)
{
	for (uint32_t attributeIndex = 0; attributeIndex < primitive.attributes_count; ++attributeIndex)
	{
		tg3_str_int_pair const& attribute = primitive.attributes[attributeIndex];
		if (AttributeNameMatches(attribute.key, attributeName))
		{
			return attribute.value;
		}
	}

	return -1;
}

bool IsValidIndex(int index, uint32_t count) { return index >= 0 && static_cast<uint32_t>(index) < count; }

bool TryGetAccessorElementPtr(
	tg3_model const* model,
	int accessorIndex,
	uint64_t elementIndex,
	uint32_t expectedType,
	int expectedComponentType,
	uint8_t const*& outElementPtr
)
{
	outElementPtr = nullptr;

	if (model == nullptr || !IsValidIndex(accessorIndex, model->accessors_count))
	{
		return false;
	}

	tg3_accessor const& accessor = model->accessors[accessorIndex];
	if (accessor.buffer_view < 0 || accessor.type != static_cast<int32_t>(expectedType)
		|| accessor.component_type != expectedComponentType || elementIndex >= accessor.count)
	{
		return false;
	}

	if (!IsValidIndex(accessor.buffer_view, model->buffer_views_count))
	{
		return false;
	}

	tg3_buffer_view const& bufferView = model->buffer_views[accessor.buffer_view];
	if (!IsValidIndex(bufferView.buffer, model->buffers_count))
	{
		return false;
	}

	tg3_buffer const& buffer = model->buffers[bufferView.buffer];
	if (buffer.data.data == nullptr)
	{
		return false;
	}

	int32_t const stride = tg3_accessor_byte_stride(&accessor, &bufferView);
	if (stride <= 0)
	{
		return false;
	}

	int32_t const componentSize  = tg3_component_size(accessor.component_type);
	int32_t const componentCount = tg3_num_components(accessor.type);
	if (componentSize <= 0 || componentCount <= 0)
	{
		return false;
	}

	uint64_t const elementSize = static_cast<uint64_t>(componentSize) * static_cast<uint64_t>(componentCount);
	uint64_t const byteOffset =
		bufferView.byte_offset + accessor.byte_offset + elementIndex * static_cast<uint64_t>(stride);
	if (byteOffset + elementSize > buffer.data.count
		|| byteOffset + elementSize > bufferView.byte_offset + bufferView.byte_length)
	{
		return false;
	}

	outElementPtr = buffer.data.data + byteOffset;
	return true;
}

bool TryReadVec3Float(tg3_model const* model, int accessorIndex, uint64_t elementIndex, Vec3& outValue)
{
	uint8_t const* elementPtr = nullptr;
	if (!TryGetAccessorElementPtr(
			model,
			accessorIndex,
			elementIndex,
			TG3_TYPE_VEC3,
			TG3_COMPONENT_TYPE_FLOAT,
			elementPtr
		))
	{
		return false;
	}

	float const* values = reinterpret_cast<float const*>(elementPtr);
	outValue            = Vec3(values[0], values[1], values[2]);
	return true;
}

bool TryReadVec2Float(tg3_model const* model, int accessorIndex, uint64_t elementIndex, Vec2& outValue)
{
	uint8_t const* elementPtr = nullptr;
	if (!TryGetAccessorElementPtr(
			model,
			accessorIndex,
			elementIndex,
			TG3_TYPE_VEC2,
			TG3_COMPONENT_TYPE_FLOAT,
			elementPtr
		))
	{
		return false;
	}

	float const* values = reinterpret_cast<float const*>(elementPtr);
	outValue            = Vec2(values[0], values[1]);
	return true;
}

bool TryReadIndex(tg3_model const* model, int accessorIndex, uint64_t elementIndex, unsigned int& outIndex)
{
	outIndex = 0;

	if (model == nullptr || !IsValidIndex(accessorIndex, model->accessors_count))
	{
		return false;
	}

	tg3_accessor const& accessor = model->accessors[accessorIndex];
	if (accessor.type != TG3_TYPE_SCALAR)
	{
		return false;
	}

	uint8_t const* elementPtr = nullptr;
	if (accessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE)
	{
		if (!TryGetAccessorElementPtr(
				model,
				accessorIndex,
				elementIndex,
				TG3_TYPE_SCALAR,
				TG3_COMPONENT_TYPE_UNSIGNED_BYTE,
				elementPtr
			))
		{
			return false;
		}

		outIndex = *reinterpret_cast<uint8_t const*>(elementPtr);
		return true;
	}

	if (accessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
	{
		if (!TryGetAccessorElementPtr(
				model,
				accessorIndex,
				elementIndex,
				TG3_TYPE_SCALAR,
				TG3_COMPONENT_TYPE_UNSIGNED_SHORT,
				elementPtr
			))
		{
			return false;
		}

		outIndex = *reinterpret_cast<uint16_t const*>(elementPtr);
		return true;
	}

	if (accessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_INT)
	{
		if (!TryGetAccessorElementPtr(
				model,
				accessorIndex,
				elementIndex,
				TG3_TYPE_SCALAR,
				TG3_COMPONENT_TYPE_UNSIGNED_INT,
				elementPtr
			))
		{
			return false;
		}

		outIndex = *reinterpret_cast<uint32_t const*>(elementPtr);
		return true;
	}

	return false;
}

bool TryGetBufferViewBytes(tg3_model const* model, int bufferViewIndex, uint8_t const*& outData, uint64_t& outByteCount)
{
	outData      = nullptr;
	outByteCount = 0;

	if (model == nullptr || !IsValidIndex(bufferViewIndex, model->buffer_views_count))
	{
		return false;
	}

	tg3_buffer_view const& bufferView = model->buffer_views[bufferViewIndex];
	if (!IsValidIndex(bufferView.buffer, model->buffers_count))
	{
		return false;
	}

	tg3_buffer const& buffer = model->buffers[bufferView.buffer];
	if (buffer.data.data == nullptr || bufferView.byte_offset + bufferView.byte_length > buffer.data.count)
	{
		return false;
	}

	outData      = buffer.data.data + bufferView.byte_offset;
	outByteCount = bufferView.byte_length;
	return true;
}

Texture* CreateTextureFromEmbeddedImage(tg3_model const* model, tg3_image const& image, std::string const& textureName)
{
	if (g_engine == nullptr || g_engine->m_renderer == nullptr || image.buffer_view < 0)
	{
		return nullptr;
	}

	uint8_t const* encodedData = nullptr;
	uint64_t encodedByteCount  = 0;
	if (!TryGetBufferViewBytes(model, image.buffer_view, encodedData, encodedByteCount) || encodedByteCount > INT_MAX)
	{
		return nullptr;
	}

	int width                         = 0;
	int height                        = 0;
	int componentCount                = 0;
	unsigned char* decodedImagePixels = nullptr;

	stbi_set_flip_vertically_on_load(true);
	decodedImagePixels =
		stbi_load_from_memory(encodedData, static_cast<int>(encodedByteCount), &width, &height, &componentCount, 4);
	stbi_set_flip_vertically_on_load(false);

	if (decodedImagePixels == nullptr)
	{
		return nullptr;
	}

	Texture* texture =
		g_engine->m_renderer->CreateTextureFromData(textureName.c_str(), IntVec2(width, height), 4, decodedImagePixels);
	stbi_image_free(decodedImagePixels);
	return texture;
}

Texture* CreateTextureFromTextureInfo(
	tg3_model const* model,
	tg3_texture_info const& textureInfo,
	std::string const& glbDirectory,
	std::string const& textureName
)
{
	if (g_engine == nullptr || g_engine->m_renderer == nullptr || model == nullptr
		|| !IsValidIndex(textureInfo.index, model->textures_count))
	{
		return nullptr;
	}

	tg3_texture const& gltfTexture = model->textures[textureInfo.index];
	if (!IsValidIndex(gltfTexture.source, model->images_count))
	{
		return nullptr;
	}

	tg3_image const& image     = model->images[gltfTexture.source];
	std::string const imageUri = ToString(image.uri);
	if (!imageUri.empty())
	{
		return g_engine->m_renderer->CreateOrGetTexture(JoinPath(glbDirectory, imageUri).c_str());
	}

	return CreateTextureFromEmbeddedImage(model, image, textureName);
}

bool LoadBaseColorTexture(
	tg3_model const* model, tg3_primitive const& primitive, char const* filePath, MeshData& outMesh
)
{
	if (model == nullptr || !IsValidIndex(primitive.material, model->materials_count))
	{
		return true;
	}

	tg3_material const& material             = model->materials[primitive.material];
	tg3_texture_info const& baseColorTexture = material.pbr_metallic_roughness.base_color_texture;
	if (baseColorTexture.index < 0)
	{
		return true;
	}

	std::string const directory   = GetDirectoryPath(filePath);
	std::string const textureName = Stringf("%s#baseColorTexture%i", filePath, baseColorTexture.index);
	outMesh.m_texture             = CreateTextureFromTextureInfo(model, baseColorTexture, directory, textureName);
	return true;
}
} // namespace

bool GLBLoader::LoadFromFile(char const* filePath, MeshData& outMesh, GLBLoadOptions const& options)
{
	outMesh.Clear();

	if (filePath == nullptr || filePath[0] == '\0')
	{
		return false;
	}

	tinygltf3::Model model;
	tinygltf3::ErrorStack errors;
	tg3_error_code const parseResult = tinygltf3::parse_file(model, errors, filePath);
	if (parseResult != TG3_OK)
	{
		return false;
	}

	if (model->meshes_count <= 0 || model->meshes[0].primitives_count <= 0)
	{
		return false;
	}

	tg3_primitive const& primitive = model->meshes[0].primitives[0];
	if (primitive.mode != TG3_MODE_TRIANGLES || primitive.indices < 0)
	{
		return false;
	}

	int const positionAccessorIndex = FindAttributeAccessor(primitive, "POSITION");
	if (positionAccessorIndex < 0 || !IsValidIndex(positionAccessorIndex, model->accessors_count))
	{
		return false;
	}

	tg3_accessor const& positionAccessor = model->accessors[positionAccessorIndex];
	if (positionAccessor.type != TG3_TYPE_VEC3 || positionAccessor.component_type != TG3_COMPONENT_TYPE_FLOAT)
	{
		return false;
	}

	int const normalAccessorIndex = FindAttributeAccessor(primitive, "NORMAL");
	int const uvAccessorIndex     = FindAttributeAccessor(primitive, "TEXCOORD_0");

	outMesh.m_vertices.reserve(static_cast<size_t>(positionAccessor.count));
	for (uint64_t vertexIndex = 0; vertexIndex < positionAccessor.count; ++vertexIndex)
	{
		Vec3 position;
		if (!TryReadVec3Float(model.get(), positionAccessorIndex, vertexIndex, position))
		{
			outMesh.Clear();
			return false;
		}

		position *= options.m_uniformScale;
		Vertex vertex(position, options.m_defaultColor);

		if (normalAccessorIndex >= 0)
		{
			Vec3 normal;
			if (TryReadVec3Float(model.get(), normalAccessorIndex, vertexIndex, normal))
			{
				vertex.m_normal = normal;
			}
		}

		if (uvAccessorIndex >= 0)
		{
			Vec2 uv;
			if (TryReadVec2Float(model.get(), uvAccessorIndex, vertexIndex, uv))
			{
				vertex.m_uvTexCoords = uv;
			}
		}

		outMesh.m_vertices.push_back(vertex);
	}

	if (!IsValidIndex(primitive.indices, model->accessors_count))
	{
		outMesh.Clear();
		return false;
	}

	tg3_accessor const& indexAccessor = model->accessors[primitive.indices];
	outMesh.m_indices.reserve(static_cast<size_t>(indexAccessor.count));
	for (uint64_t indexIndex = 0; indexIndex < indexAccessor.count; ++indexIndex)
	{
		unsigned int index = 0;
		if (!TryReadIndex(model.get(), primitive.indices, indexIndex, index))
		{
			outMesh.Clear();
			return false;
		}

		outMesh.m_indices.push_back(index);
	}

	LoadBaseColorTexture(model.get(), primitive, filePath, outMesh);
	return !outMesh.IsEmpty();
}

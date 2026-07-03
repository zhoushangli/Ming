#include "MingEngine/Scene/Resource/MeshResourceFormat.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include <cstdint>
#include <cstring>
#include <exception>
#include <sstream>

namespace
{
using Json = nlohmann::ordered_json;

constexpr char const* kMeshMagic     = "MESH";
constexpr uint32_t    kMeshFileVersion  = 1;
constexpr size_t      kMinHeaderSize = 4096;
constexpr char const* kMeshExtension = ".mesh";

struct BinaryBlock
{
	size_t m_offset = 0;
	size_t m_size   = 0;
};

bool HasExtension(std::string const& virtualPath, std::string const& extension)
{
	return virtualPath.size() >= extension.size()
		&& virtualPath.compare(virtualPath.size() - extension.size(), extension.size(), extension) == 0;
}

std::string MakePrelude(size_t headerSize)
{
	std::ostringstream stream;
	stream << kMeshMagic << " version=" << kMeshFileVersion << " header_size=" << headerSize << "\n";
	return stream.str();
}

bool TryParsePrelude(std::string const& prelude, uint32_t& outVersion, size_t& outHeaderSize)
{
	outVersion    = 0;
	outHeaderSize = 0;

	std::istringstream stream(prelude);
	std::string        magic;
	std::string        versionToken;
	std::string        headerSizeToken;
	if (!(stream >> magic >> versionToken >> headerSizeToken) || magic != kMeshMagic)
	{
		return false;
	}

	constexpr char const* versionPrefix    = "version=";
	constexpr char const* headerSizePrefix = "header_size=";
	if (versionToken.compare(0, std::strlen(versionPrefix), versionPrefix) != 0
		|| headerSizeToken.compare(0, std::strlen(headerSizePrefix), headerSizePrefix) != 0)
	{
		return false;
	}

	try
	{
		outVersion    = static_cast<uint32_t>(std::stoul(versionToken.substr(std::strlen(versionPrefix))));
		outHeaderSize = static_cast<size_t>(std::stoull(headerSizeToken.substr(std::strlen(headerSizePrefix))));
		return true;
	}
	catch (std::exception const&)
	{
		return false;
	}
}

bool IsValidMeshData(ImportMeshData const& meshData)
{
	if (meshData.m_vertexFormat.empty() || meshData.m_vertexStride == 0 || meshData.m_vertexCount == 0)
	{
		return false;
	}

	if (meshData.m_indexFormat.empty() || meshData.m_indexStride == 0 || meshData.m_indexCount == 0)
	{
		return false;
	}

	size_t const vertexByteCount = static_cast<size_t>(meshData.m_vertexStride) * meshData.m_vertexCount;
	size_t const indexByteCount  = static_cast<size_t>(meshData.m_indexStride) * meshData.m_indexCount;
	return meshData.m_vertices.size() == vertexByteCount && meshData.m_indices.size() == indexByteCount;
}

Json MakeBlockJson(BinaryBlock const& block)
{
	Json json;
	json["offset"] = block.m_offset;
	json["size"]   = block.m_size;
	return json;
}

BinaryBlock AppendPayload(std::vector<uint8_t>& payload, std::vector<uint8_t> const& data)
{
	BinaryBlock block;
	block.m_offset = payload.size();
	block.m_size   = data.size();
	payload.insert(payload.end(), data.begin(), data.end());
	return block;
}

bool TryReadBlock(Json const& json, size_t payloadSize, BinaryBlock& outBlock)
{
	outBlock = BinaryBlock();

	if (!json.is_object() || !json.contains("offset") || !json.contains("size")
		|| !json["offset"].is_number_unsigned() || !json["size"].is_number_unsigned())
	{
		return false;
	}

	outBlock.m_offset = json["offset"].get<size_t>();
	outBlock.m_size   = json["size"].get<size_t>();
	return outBlock.m_offset <= payloadSize && outBlock.m_size <= payloadSize - outBlock.m_offset;
}

bool TryReadString(Json const& json, char const* name, std::string& outValue)
{
	if (!json.contains(name) || !json[name].is_string())
	{
		return false;
	}

	outValue = json[name].get<std::string>();
	return true;
}

bool TryReadUInt32(Json const& json, char const* name, uint32_t& outValue)
{
	if (!json.contains(name) || !json[name].is_number_unsigned())
	{
		return false;
	}

	outValue = json[name].get<uint32_t>();
	return true;
}

void CopyPayloadBlock(
	std::vector<uint8_t> const& payload,
	BinaryBlock const&         block,
	std::vector<uint8_t>&       outData)
{
	outData.resize(block.m_size);
	if (block.m_size > 0)
	{
		memcpy(outData.data(), payload.data() + block.m_offset, block.m_size);
	}
}

} // namespace

std::vector<std::string> MeshResourceLoader::GetSupportedExtensions() const { return { kMeshExtension }; }

Ref<Resource> MeshResourceLoader::Load(std::string const& virtualPath)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return Ref<Resource>();
	}

	std::vector<uint8_t> fileData;
	if (!g_engine->m_fileSystem->ReadBinary(virtualPath, fileData))
	{
		return Ref<Resource>();
	}

	auto const newlineIt = std::find(fileData.begin(), fileData.end(), static_cast<uint8_t>('\n'));
	if (newlineIt == fileData.end())
	{
		return Ref<Resource>();
	}

	size_t const      preludeSize = static_cast<size_t>(newlineIt - fileData.begin()) + 1;
	std::string const prelude(reinterpret_cast<char const*>(fileData.data()), preludeSize);

	uint32_t version    = 0;
	size_t   headerSize = 0;
	if (!TryParsePrelude(prelude, version, headerSize) || version != kMeshFileVersion
		|| headerSize <= preludeSize || headerSize > fileData.size())
	{
		return Ref<Resource>();
	}

	size_t const           payloadOffset = headerSize;
	std::string const      jsonText(reinterpret_cast<char const*>(fileData.data() + preludeSize), headerSize - preludeSize);
	std::vector<uint8_t>   payload(fileData.begin() + payloadOffset, fileData.end());
	size_t const           payloadSize = payload.size();

	try
	{
		Json root = Json::parse(jsonText);
		if (!root.is_object() || !root.contains("type") || root["type"].get<std::string>() != "Mesh")
		{
			return Ref<Resource>();
		}

		Ref<ImportMeshData> meshData = CreateRef<ImportMeshData>();
		if (!TryReadString(root, "name", meshData->m_name)
			|| !TryReadString(root, "vertex_format", meshData->m_vertexFormat)
			|| !TryReadUInt32(root, "vertex_stride", meshData->m_vertexStride)
			|| !TryReadUInt32(root, "vertex_count", meshData->m_vertexCount)
			|| !TryReadString(root, "index_format", meshData->m_indexFormat)
			|| !TryReadUInt32(root, "index_stride", meshData->m_indexStride)
			|| !TryReadUInt32(root, "index_count", meshData->m_indexCount))
		{
			return Ref<Resource>();
		}

		BinaryBlock verticesBlock;
		BinaryBlock indicesBlock;
		if (!TryReadBlock(root["vertices"], payloadSize, verticesBlock)
			|| !TryReadBlock(root["indices"], payloadSize, indicesBlock))
		{
			return Ref<Resource>();
		}

		if (verticesBlock.m_size != static_cast<size_t>(meshData->m_vertexStride) * meshData->m_vertexCount
			|| indicesBlock.m_size != static_cast<size_t>(meshData->m_indexStride) * meshData->m_indexCount)
		{
			return Ref<Resource>();
		}

		CopyPayloadBlock(payload, verticesBlock, meshData->m_vertices);
		CopyPayloadBlock(payload, indicesBlock, meshData->m_indices);

		if (root.contains("textures"))
		{
			if (!root["textures"].is_array())
			{
				return Ref<Resource>();
			}

			for (Json const& textureJson : root["textures"])
			{
				ImportTextureData textureData;
				if (!TryReadString(textureJson, "name", textureData.m_name)
					|| !TryReadString(textureJson, "format", textureData.m_format)
					|| !TryReadUInt32(textureJson, "width", textureData.m_width)
					|| !TryReadUInt32(textureJson, "height", textureData.m_height)
					|| !TryReadUInt32(textureJson, "channels", textureData.m_channels))
				{
					return Ref<Resource>();
				}

				BinaryBlock textureBlock;
				if (!TryReadBlock(textureJson["data"], payloadSize, textureBlock))
				{
					return Ref<Resource>();
				}

				CopyPayloadBlock(payload, textureBlock, textureData.m_data);
				meshData->m_textures.push_back(std::move(textureData));
			}
		}

		if (!IsValidMeshData(*meshData))
		{
			return Ref<Resource>();
		}

		meshData->SetVirtualPath(virtualPath);
		meshData->SetName(meshData->m_name);
		return meshData;
	}
	catch (std::exception const&)
	{
		return Ref<Resource>();
	}
}

bool MeshResourceSaver::CanSave(std::string const& virtualPath, Variant const& value) const
{
	Ref<ImportMeshData> meshData(value);
	return meshData.IsValid() && HasExtension(virtualPath, kMeshExtension);
}

bool MeshResourceSaver::Save(std::string const& virtualPath, Variant const& value)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr || !FileSystem::IsVirtualPath(virtualPath))
	{
		return false;
	}

	Ref<ImportMeshData> meshData(value);
	if (!meshData.IsValid() || !IsValidMeshData(*meshData))
	{
		return false;
	}

	std::vector<uint8_t> payload;
	BinaryBlock const   verticesBlock = AppendPayload(payload, meshData->m_vertices);
	BinaryBlock const   indicesBlock  = AppendPayload(payload, meshData->m_indices);

	Json root;
	root["type"]          = "Mesh";
	root["version"]       = kMeshFileVersion;
	root["name"]          = meshData->m_name;
	root["vertex_format"] = meshData->m_vertexFormat;
	root["vertex_stride"] = meshData->m_vertexStride;
	root["vertex_count"]  = meshData->m_vertexCount;
	root["vertices"]      = MakeBlockJson(verticesBlock);
	root["index_format"]  = meshData->m_indexFormat;
	root["index_stride"]  = meshData->m_indexStride;
	root["index_count"]   = meshData->m_indexCount;
	root["indices"]       = MakeBlockJson(indicesBlock);
	root["textures"]      = Json::array();

	for (ImportTextureData const& textureData : meshData->m_textures)
	{
		BinaryBlock const textureBlock = AppendPayload(payload, textureData.m_data);

		Json textureJson;
		textureJson["name"]     = textureData.m_name;
		textureJson["format"]   = textureData.m_format;
		textureJson["width"]    = textureData.m_width;
		textureJson["height"]   = textureData.m_height;
		textureJson["channels"] = textureData.m_channels;
		textureJson["data"]     = MakeBlockJson(textureBlock);
		root["textures"].push_back(textureJson);
	}

	std::string const jsonText = root.dump(1, '\t');
	size_t headerSize = kMinHeaderSize;
	while (MakePrelude(headerSize).size() + jsonText.size() > headerSize)
	{
		headerSize *= 2;
	}

	std::string const prelude = MakePrelude(headerSize);

	std::vector<uint8_t> fileData;
	fileData.resize(headerSize + payload.size(), static_cast<uint8_t>(' '));
	memcpy(fileData.data(), prelude.data(), prelude.size());
	memcpy(fileData.data() + prelude.size(), jsonText.data(), jsonText.size());
	if (!payload.empty())
	{
		memcpy(fileData.data() + headerSize, payload.data(), payload.size());
	}

	if (!g_engine->m_fileSystem->WriteBinary(virtualPath, fileData))
	{
		return false;
	}

	meshData->SetVirtualPath(virtualPath);
	meshData->SetName(meshData->m_name);
	return true;
}

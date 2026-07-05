#include "MingEngine/Scene/Resource/TextureResourceFormat.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <sstream>

namespace
{
using Json = nlohmann::ordered_json;

constexpr char const* kTexMagic       = "TEX";
constexpr uint32_t    kTexFileVersion = 1;
constexpr size_t      kMinHeaderSize  = 4096;
constexpr char const* kTexExtension   = ".tex";

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
	stream << kTexMagic << " version=" << kTexFileVersion << " header_size=" << headerSize << "\n";
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
	if (!(stream >> magic >> versionToken >> headerSizeToken) || magic != kTexMagic)
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

	if (!json.is_object() || !json.contains("offset") || !json.contains("size") || !json["offset"].is_number_unsigned()
		|| !json["size"].is_number_unsigned())
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

void CopyPayloadBlock(std::vector<uint8_t> const& payload, BinaryBlock const& block, std::vector<uint8_t>& outData)
{
	outData.resize(block.m_size);
	if (block.m_size > 0)
	{
		memcpy(outData.data(), payload.data() + block.m_offset, block.m_size);
	}
}

bool IsValidTextureData(TextureResource const& texData)
{
	return texData.m_dimensions.x > 0 && texData.m_dimensions.y > 0 && texData.m_channels > 0
		   && texData.m_pixels.size()
				  == static_cast<size_t>(texData.m_dimensions.x) * texData.m_dimensions.y * texData.m_channels;
}

} // namespace

std::vector<std::string> TextureResourceLoader::GetSupportedExtensions() const { return { kTexExtension }; }

Ref<Resource> TextureResourceLoader::Load(std::string const& virtualPath)
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
	if (!TryParsePrelude(prelude, version, headerSize) || version != kTexFileVersion || headerSize <= preludeSize
		|| headerSize > fileData.size())
	{
		return Ref<Resource>();
	}

	size_t const      payloadOffset = headerSize;
	std::string const jsonText(reinterpret_cast<char const*>(fileData.data() + preludeSize), headerSize - preludeSize);
	std::vector<uint8_t> payload(fileData.begin() + payloadOffset, fileData.end());
	size_t const         payloadSize = payload.size();

	try
	{
		Json root = Json::parse(jsonText);
		if (!root.is_object() || !root.contains("type") || root["type"].get<std::string>() != "Texture")
		{
			return Ref<Resource>();
		}

		std::string          texName;
		Ref<TextureResource> texData = CreateRef<TextureResource>();
		if (!TryReadString(root, "name", texName) || !TryReadString(root, "format", texData->m_format)
			|| !TryReadUInt32(root, "width", (uint32_t&)texData->m_dimensions.x)
			|| !TryReadUInt32(root, "height", (uint32_t&)texData->m_dimensions.y)
			|| !TryReadUInt32(root, "channels", (uint32_t&)texData->m_channels))
		{
			return Ref<Resource>();
		}

		BinaryBlock dataBlock;
		if (!TryReadBlock(root["data"], payloadSize, dataBlock))
		{
			return Ref<Resource>();
		}

		CopyPayloadBlock(payload, dataBlock, texData->m_pixels);

		if (!IsValidTextureData(*texData))
		{
			return Ref<Resource>();
		}

		texData->SetVirtualPath(virtualPath);
		texData->SetName(texName);

		texData->InitGPUResources();

		return texData;
	}
	catch (std::exception const&)
	{
		return Ref<Resource>();
	}
}

bool TextureResourceSaver::CanSave(std::string const& virtualPath, Variant const& value) const
{
	Ref<TextureResource> texData(value);
	return texData.IsValid() && HasExtension(virtualPath, kTexExtension);
}

bool TextureResourceSaver::Save(std::string const& virtualPath, Variant const& value)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr || !FileSystem::IsVirtualPath(virtualPath))
	{
		return false;
	}

	Ref<TextureResource> texData(value);
	if (!texData.IsValid() || !IsValidTextureData(*texData))
	{
		return false;
	}

	std::vector<uint8_t> payload;
	BinaryBlock const    dataBlock = AppendPayload(payload, texData->m_pixels);

	Json root;
	root["type"]     = "Texture";
	root["version"]  = kTexFileVersion;
	root["name"]     = texData->GetName();
	root["format"]   = texData->m_format;
	root["width"]    = texData->m_dimensions.x;
	root["height"]   = texData->m_dimensions.y;
	root["channels"] = texData->m_channels;
	root["data"]     = MakeBlockJson(dataBlock);

	std::string const jsonText   = root.dump(1, '\t');
	size_t            headerSize = kMinHeaderSize;
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

	texData->SetVirtualPath(virtualPath);
	return true;
}

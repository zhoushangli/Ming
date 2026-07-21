#include "MingEngine/Scene/Import/ImageImporter.hpp"

#include "MingEngine/Core/Image.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <utility>

std::vector<std::string> ImageImporter::GetSupportedExtensions() const
{
	return { "png", "jpg", "jpeg", "bmp", "tga", "hdr" };
}

std::string ImageImporter::GetVisibleName() const { return "ImageImporter"; }

std::string ImageImporter::GetImportedExtension() const { return "tex"; }

std::vector<ImportOptions> const ImageImporter::GetImportOptions() const { return {}; }

Ref<Resource> ImageImporter::Import(
	std::unordered_map<std::string, Variant> const& importOptions, VirtualPath const& sourceVirtualPath)
{
	importOptions;

	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return Ref<Resource>();
	}

	// 1) Resolve the source name and read its encoded bytes once
	std::filesystem::path physicalPath;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(sourceVirtualPath, physicalPath))
	{
		return Ref<Resource>();
	}

	std::vector<uint8_t> encodedData;
	if (!g_engine->m_fileSystem->ReadBinary(sourceVirtualPath, encodedData))
	{
		return Ref<Resource>();
	}

	// 2) Retain and decode the encoded source data in Image
	Ref<Image> image = CreateRef<Image>();
	if (!image->LoadFromMemory(std::move(encodedData)))
	{
		return Ref<Resource>();
	}

	// 3) Store the complete CPU image in TextureResource
	Ref<TextureResource> texData = CreateRef<TextureResource>();
	texData->SetName(physicalPath.stem().string());
	texData->m_image = std::move(image);

	return texData;
}

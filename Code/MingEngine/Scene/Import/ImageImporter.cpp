#include "MingEngine/Scene/Import/ImageImporter.hpp"

#include "MingEngine/Core/Image.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include "ThirdParty/stb/stb_image.h"

#include <cstring>

std::vector<std::string> ImageImporter::GetSupportedExtensions() const
{
	return { "png", "jpg", "jpeg", "bmp", "tga", "hdr" };
}

std::string ImageImporter::GetVisibleName() const { return "ImageImporter"; }

std::string ImageImporter::GetImportedExtension() const { return "tex"; }

std::vector<ImportOptions> const ImageImporter::GetImportOptions() const { return {}; }

Ref<Resource> ImageImporter::Import(
	std::unordered_map<std::string, Variant> const& importOptions, std::string const& sourceVirtualPath)
{
	importOptions;

	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return Ref<Resource>();
	}

	// 1) Load image from file via Image helper (stb_image)
	std::filesystem::path physicalPath;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(sourceVirtualPath, physicalPath))
	{
		return Ref<Resource>();
	}

	Image image(physicalPath.string());
	if (!image.IsValid())
	{
		return Ref<Resource>();
	}

	// 2) Convert Image's Rgba8 data to raw uint8_t binary
	IntVec2 const dims = image.GetDimensions();

	// 3) Fill TextureResource CPU fields
	Ref<TextureResource> texData = CreateRef<TextureResource>();
	texData->m_dimensions        = dims;
	texData->m_channels          = 4;
	texData->m_format            = "RGBA8";
	texData->m_pixels.assign(
		static_cast<uint8_t const*>(image.GetRawData()),
		static_cast<uint8_t const*>(image.GetRawData()) + dims.x * dims.y * 4);

	return texData;
}

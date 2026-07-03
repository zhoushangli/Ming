#include "MingEngine/Core/Image.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <ThirdParty/stb/stb_image.h>

Image::Image() { m_dimensions = IntVec2::Zero; }

Image::~Image() {}

Image::Image(IntVec2 size, Rgba8 color)
{
	GUARANTEE_OR_DIE(Initialize(size, color), "Image: invalid dimensions");
}

Image::Image(char const* imageFilePath)
{
	GUARANTEE_OR_DIE(imageFilePath != nullptr && imageFilePath[0] != '\0', "Image: imageFilePath is null/empty");
	GUARANTEE_OR_DIE(LoadFromFile(imageFilePath), Stringf("Failed to load image from file: %s", imageFilePath));
}

Image::Image(std::string const& imageFilePath) : Image(imageFilePath.c_str()) {}

void Image::Clear()
{
	m_imageFilePath.clear();
	m_texelColors.clear();
	m_dimensions = IntVec2::Zero;
}

bool Image::IsValid() const { return m_dimensions.x > 0 && m_dimensions.y > 0 && !m_texelColors.empty(); }

bool Image::Initialize(IntVec2 size, Rgba8 color)
{
	Clear();
	if (size.x <= 0 || size.y <= 0)
	{
		return false;
	}

	m_dimensions = size;

	int const totalTexels = m_dimensions.x * m_dimensions.y;
	m_texelColors.assign(static_cast<size_t>(totalTexels), color);
	return true;
}

bool Image::LoadFromFile(std::string const& imageFilePath)
{
	Clear();
	if (imageFilePath.empty())
	{
		return false;
	}

	int numComponents = 0;

	stbi_set_flip_vertically_on_load(true);
	unsigned char* imageData = stbi_load(imageFilePath.c_str(), &m_dimensions.x, &m_dimensions.y, &numComponents, 4);
	stbi_set_flip_vertically_on_load(false);

	if (imageData == nullptr)
	{
		Clear();
		return false;
	}

	int totalTexels = m_dimensions.x * m_dimensions.y;
	m_texelColors.reserve(totalTexels);
	m_imageFilePath = imageFilePath;

	for (int texelIndex = 0; texelIndex < totalTexels; ++texelIndex)
	{
		int           byteIndex = texelIndex * 4;
		unsigned char r         = imageData[byteIndex + 0];
		unsigned char g         = imageData[byteIndex + 1];
		unsigned char b         = imageData[byteIndex + 2];
		unsigned char a         = (numComponents < 4) ? 255 : imageData[byteIndex + 3];
		m_texelColors.emplace_back(r, g, b, a);
	}

	stbi_image_free(imageData);
	return true;
}

Rgba8 Image::GetColorAt(int x, int y) const
{
	GUARANTEE_OR_DIE(x >= 0 && x < m_dimensions.x && y >= 0 && y < m_dimensions.y, "GetColorAt out of bounds");
	int const index = y * m_dimensions.x + x;
	return m_texelColors[static_cast<size_t>(index)];
}

const std::string& Image::GetImageFilePath() const { return m_imageFilePath; }

const void* Image::GetRawData() const { return m_texelColors.empty() ? nullptr : m_texelColors.data(); }


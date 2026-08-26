#include "MingEngine/Core/Image.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Render/Color.hpp"

#include "ThirdParty/stb/stb_image.h"

#include <climits>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <utility>

bool Image::LoadFromFile(std::string const& physicalPath)
{
	// 1) Read the encoded source file once
	std::ifstream file(physicalPath, std::ios::binary | std::ios::ate);
	if (!file)
	{
		return false;
	}

	std::streamoff const fileSize = file.tellg();
	if (fileSize <= 0 || fileSize > INT_MAX)
	{
		return false;
	}

	std::vector<uint8_t> encodedData(static_cast<size_t>(fileSize));
	file.seekg(0, std::ios::beg);
	if (!file.read(reinterpret_cast<char*>(encodedData.data()), static_cast<std::streamsize>(encodedData.size())))
	{
		return false;
	}

	// 2) Retain and decode the same source bytes
	return LoadFromMemory(std::move(encodedData));
}

bool Image::LoadFromMemory(std::vector<uint8_t> encodedData)
{
	// 1) Validate the encoded input accepted by stb_image
	if (encodedData.empty() || encodedData.size() > INT_MAX)
	{
		return false;
	}

	int width          = 0;
	int height         = 0;
	int channelsInFile = 0;

	// 2) Decode into temporary RGBA8 storage
	stbi_set_flip_vertically_on_load(true);
	std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> decodedPixels(
		stbi_load_from_memory(
			encodedData.data(),
			static_cast<int>(encodedData.size()),
			&width,
			&height,
			&channelsInFile,
			STBI_rgb_alpha),
		&stbi_image_free);
	stbi_set_flip_vertically_on_load(false);

	if (decodedPixels == nullptr || width <= 0 || height <= 0)
	{
		return false;
	}

	size_t const pixelWidth  = static_cast<size_t>(width);
	size_t const pixelHeight = static_cast<size_t>(height);
	if (pixelWidth > std::numeric_limits<size_t>::max() / pixelHeight
		|| pixelWidth * pixelHeight > std::numeric_limits<size_t>::max() / STBI_rgb_alpha)
	{
		return false;
	}

	size_t const         pixelDataSize = pixelWidth * pixelHeight * STBI_rgb_alpha;
	std::vector<uint8_t> pixels(pixelDataSize);
	memcpy(pixels.data(), decodedPixels.get(), pixelDataSize);

	// 3) Commit encoded and decoded data only after the full load succeeds
	m_dimensions  = IntVec2(width, height);
	m_channels    = STBI_rgb_alpha;
	m_encodedData = std::move(encodedData);
	m_pixels      = std::move(pixels);
	return true;
}

void Image::Clear()
{
	m_dimensions = IntVec2::Zero;
	m_channels   = 4;
	m_encodedData.clear();
	m_pixels.clear();
}

bool Image::IsValid() const
{
	if (m_dimensions.x <= 0 || m_dimensions.y <= 0 || m_channels != 4)
	{
		return false;
	}

	size_t const pixelWidth  = static_cast<size_t>(m_dimensions.x);
	size_t const pixelHeight = static_cast<size_t>(m_dimensions.y);
	if (pixelWidth > std::numeric_limits<size_t>::max() / pixelHeight
		|| pixelWidth * pixelHeight > std::numeric_limits<size_t>::max() / m_channels)
	{
		return false;
	}

	size_t const pixelDataSize = pixelWidth * pixelHeight * m_channels;
	return m_pixels.size() == pixelDataSize;
}

bool Image::HasEncodedData() const { return !m_encodedData.empty(); }

Color Image::GetColorAt(int x, int y) const
{
	GUARANTEE_OR_DIE(x >= 0 && x < m_dimensions.x && y >= 0 && y < m_dimensions.y, "GetColorAt out of bounds");

	size_t const byteIndex = (static_cast<size_t>(y) * m_dimensions.x + x) * m_channels;
	return Color(m_pixels[byteIndex], m_pixels[byteIndex + 1], m_pixels[byteIndex + 2], m_pixels[byteIndex + 3]);
}

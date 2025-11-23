#include "Engine/Renderer/Image.hpp"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include <ThirdParty/stb/stb_image.h>

Image::Image(char const* imageFilePath)
{
    int numComponents = 0;
    unsigned char* imageData = stbi_load(imageFilePath, &m_dimensions.x, &m_dimensions.y, &numComponents, STBI_rgb_alpha);

    // GUARANTEE_OR_DIE(imageData != nullptr, "Failed to load image from file: %s", imageFilePath);

    int totalTexels = m_dimensions.x * m_dimensions.y;
    m_texelColors.reserve(totalTexels);

    for (int texelIndex = 0; texelIndex < totalTexels; ++texelIndex)
    {
        int byteIndex = texelIndex * numComponents;
        unsigned char r = imageData[byteIndex + 0];
        unsigned char g = imageData[byteIndex + 1];
        unsigned char b = imageData[byteIndex + 2];
        unsigned char a = (numComponents < 4) ? 255 : imageData[byteIndex + 3];
        m_texelColors.emplace_back(r, g, b, a);
    }

    stbi_image_free(imageData);
}

Rgba8 Image::GetColorAt(int x, int y) const
{
    int index = y * m_dimensions.x + x;
    GUARANTEE_OR_DIE(index >= 0 && index < static_cast<int>(m_texelColors.size()), "GetColorAt out of bounds");
    return m_texelColors[index];
}


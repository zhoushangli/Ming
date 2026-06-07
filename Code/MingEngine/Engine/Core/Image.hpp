#pragma once

#include "MingEngine/Engine/Math/IntVec2.hpp"

#include <vector>
#include <string>

struct Rgba8;

class Image
{
public:
    Image();
    ~Image();
    Image(IntVec2 size, Rgba8 color);
    Image(char const* imageFilePath);
    Image(std::string const& imageFilePath);

    Rgba8 GetColorAt(int x, int y) const;

    IntVec2 GetDimensions() const { return m_dimensions; }

    const std::string& GetImageFilePath() const;
    const void* GetRawData() const;

protected:
    std::string m_imageFilePath;
    std::vector<Rgba8> m_texelColors;
    IntVec2 m_dimensions;
};

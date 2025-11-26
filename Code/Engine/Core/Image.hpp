#pragma once

#include "Engine/Math/IntVec2.hpp"

#include <vector>
#include <string>

struct Rgba8;

class Image
{
public:
    Image(char const* imageFilePath);
    Image(std::string const& imageFilePath);

    Rgba8 GetColorAt(int x, int y) const;

    IntVec2 GetDimensions() const { return m_dimensions; }

protected:
    std::vector<Rgba8> m_texelColors;
    IntVec2 m_dimensions;
};
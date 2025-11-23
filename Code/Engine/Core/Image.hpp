#pragma once

#include "Engine/Math/IntVec2.hpp"

#include <vector>

struct Rgba8;

class Image
{
public:
    Image(char const* imageFilePath);

    Rgba8 GetColorAt(int x, int y) const;

protected:
    std::vector<Rgba8> m_texelColors;
    IntVec2 m_dimensions;
};
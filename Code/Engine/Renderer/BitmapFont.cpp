#include "Engine/Renderer/BitmapFont.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec2.hpp"

#include <string>
#include <vector>

int const FONT_SPRITE_SHEET_COLS = 16;
int const FONT_SPRITE_SHEET_ROWS = 16;

BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture)
    : m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension)
    , m_fontGlyphsSpriteSheet(fontTexture, IntVec2(FONT_SPRITE_SHEET_COLS, FONT_SPRITE_SHEET_ROWS))
{
    IntVec2 texDims = fontTexture.GetDimensions();
    float texAspect = (texDims.y == 0) ? 1.f : (float)texDims.x / (float)texDims.y;
    float gridAspect = (float)FONT_SPRITE_SHEET_COLS / (float)FONT_SPRITE_SHEET_ROWS;
    m_fontDefaultAspect = texAspect / gridAspect;
}

Texture& BitmapFont::GetTexture()
{
    return m_fontGlyphsSpriteSheet.GetTexture();
}

void BitmapFont::AddVertsForText2D(
    std::vector<Vertex>& vertexArray,
    Vec2 textMins,
    float cellHeight,
    std::string const& text,
    Rgba8 tint,
    float cellAspectScale)
{
    float cellWidth = cellHeight * m_fontDefaultAspect * cellAspectScale;
    Vec2 pen = textMins;

    for (char c : text)
    {
        int index = static_cast<int>(c);
        AABB2 uv = m_fontGlyphsSpriteSheet.GetSpriteUVs(index);

        AABB2 bounds(pen, pen + Vec2(cellWidth, cellHeight));

        vertexArray.emplace_back(Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.f), tint, uv.m_mins);
        vertexArray.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_mins.y, 0.f), tint, Vec2(uv.m_maxs.x, uv.m_mins.y));
        vertexArray.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.f), tint, uv.m_maxs);

        vertexArray.emplace_back(Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.f), tint, uv.m_mins);
        vertexArray.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.f), tint, uv.m_maxs);
        vertexArray.emplace_back(Vec3(bounds.m_mins.x, bounds.m_maxs.y, 0.f), tint, Vec2(uv.m_mins.x, uv.m_maxs.y));

        pen.x += cellWidth;
    }
}

float BitmapFont::GetTextWidth(float cellHeight, std::string const& text, float cellAspectScale)
{
    float cellWidth = cellHeight * m_fontDefaultAspect * cellAspectScale;
    return cellWidth * static_cast<float>(text.length());
}

float BitmapFont::GetGlyphAspect(int /*glyphUnicode*/) const
{
    return m_fontDefaultAspect;
}
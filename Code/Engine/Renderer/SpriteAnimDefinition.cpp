#include "Engine/Renderer/Texture.hpp"

#include "Engine/Math/AABB2.hpp"

// Texture
Texture::Texture()
{
}

Texture::~Texture()
{
}

// SpriteDefinition
SpriteDefinition::SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs)
    : m_spriteSheet(spriteSheet)
    , m_spriteIndex(spriteIndex)
    , m_uvAtMins(uvAtMins)
    , m_uvAtMaxs(uvAtMaxs)
{
}

void SpriteDefinition::GetUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs) const
{
    out_uvAtMins = m_uvAtMins;
    out_uvAtMaxs = m_uvAtMaxs;
}

AABB2 SpriteDefinition::GetUVs() const
{
    return AABB2(m_uvAtMins, m_uvAtMaxs);
}

SpriteSheet const& SpriteDefinition::GetSpriteSheet() const
{
    return m_spriteSheet;
}

Texture& SpriteDefinition::GetTexture() const
{
    return m_spriteSheet.GetTexture();
}

float SpriteDefinition::GetAspect() const
{
    // Calculate aspect ratio from UVs
    float width = m_uvAtMaxs.x - m_uvAtMins.x;
    float height = m_uvAtMaxs.y - m_uvAtMins.y;
    return (height != 0.0f) ? (width / height) : 1.0f;
}

// SpriteSheet
SpriteSheet::SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout)
    : m_texture(texture)
{
    int numSprites = simpleGridLayout.x * simpleGridLayout.y;
    m_spriteDefs.reserve(numSprites);

    float cellWidth = 1.0f / static_cast<float>(simpleGridLayout.x);
    float cellHeight = 1.0f / static_cast<float>(simpleGridLayout.y);

    float texelWidth = 1.0f / static_cast<float>(m_texture.GetDimensions().x);
    float texelHeight = 1.0f / static_cast<float>(m_texture.GetDimensions().y);
    Vec2 texelOffset(texelWidth / 128.f, texelHeight / 128.f);

    for (int y = simpleGridLayout.y - 1; y >= 0; --y) {
        for (int x = 0; x < simpleGridLayout.x; ++x) {
            int spriteIndex = y * simpleGridLayout.x + x;
            Vec2 uvMins(cellWidth * x, cellHeight * y);
            Vec2 uvMaxs(cellWidth * (x + 1), cellHeight * (y + 1));
            uvMins += texelOffset;
            uvMaxs -= texelOffset;
            m_spriteDefs.emplace_back(*this, spriteIndex, uvMins, uvMaxs);
        }
    }
}

Texture& SpriteSheet::GetTexture() const
{
    return m_texture;
}

int SpriteSheet::GetNumSprites() const
{
    return static_cast<int>(m_spriteDefs.size());
}

SpriteDefinition const& SpriteSheet::GetSpriteDef(int spriteIndex) const
{
    return m_spriteDefs[spriteIndex];
}

void SpriteSheet::GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const
{
    m_spriteDefs[spriteIndex].GetUVs(out_uvAtMins, out_uvAtMaxs);
}

AABB2 SpriteSheet::GetSpriteUVs(int spriteIndex) const
{
    return m_spriteDefs[spriteIndex].GetUVs();
}


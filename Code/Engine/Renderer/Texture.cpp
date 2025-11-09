#include "Engine/Renderer/Texture.hpp"

#include "Engine/Math/AABB2.hpp"

Texture::Texture()
{

}

Texture::~Texture()
{

}

SpriteDef::SpriteDef(AABB2 const& uvBounds) : m_uvBounds(uvBounds)
{

}

SpriteSheet::SpriteSheet(Texture const& texture, IntVec2 const& gridLayout) : m_texture(texture)
{
    int numSprites = gridLayout.x * gridLayout.y;
    m_spriteDefs.reserve(numSprites);

    for (int y = gridLayout.y - 1; y >= 0; --y)
    {
        for (int x = 0; x < gridLayout.x; ++x)
        {
            float uMin = static_cast<float>(x) / static_cast<float>(gridLayout.x);
            float vMin = static_cast<float>(y) / static_cast<float>(gridLayout.y);
            float uMax = static_cast<float>(x + 1) / static_cast<float>(gridLayout.x);
            float vMax = static_cast<float>(y + 1) / static_cast<float>(gridLayout.y);
            AABB2 uvBounds(Vec2(uMin, vMin), Vec2(uMax, vMax));
            m_spriteDefs.emplace_back(uvBounds);
        }
    }
}


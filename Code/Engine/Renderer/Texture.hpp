#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"

#include <string>
#include <vector>

class Texture
{
	friend class Renderer; // Only the Renderer can create new Texture objects!

private:
	Texture(); // can't instantiate directly; must ask Renderer to do it for you
	Texture(Texture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~Texture();

public:
	IntVec2				GetDimensions() const { return m_dimensions; }
	std::string const&  GetImageFilePath() const { return m_name; }

protected:
	std::string			m_name;			// Can't be char const* -- store a copy, in case it was temporary
	IntVec2				m_dimensions;

	// #ToDo in SD2: Use #if defined( ENGINE_RENDER_D3D11 ) to do something different for DX11; #else do:
	unsigned int		m_textureID = 0xFFFFFFFF;
};

class SpriteSheet;

class SpriteDefinition
{
public:
    explicit SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs);

    void                GetUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs) const;
    AABB2               GetUVs() const;
    SpriteSheet const&  GetSpriteSheet() const;
    Texture&            GetTexture() const;
    float               GetAspect() const;

protected:
    SpriteSheet const& m_spriteSheet;
    int                  m_spriteIndex = -1;
    Vec2                 m_uvAtMins = Vec2::ZERO;
    Vec2                 m_uvAtMaxs = Vec2::ONE;
};

class SpriteSheet
{
public:
    explicit SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout);

    Texture& GetTexture() const;
    int                     GetNumSprites() const;
    SpriteDefinition const& GetSpriteDef(int spriteIndex) const;
    void                    GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const;
    AABB2                   GetSpriteUVs(int spriteIndex) const;

protected:
    Texture& m_texture;      // reference members must be set in constructor's initializer list
    std::vector<SpriteDefinition>    m_spriteDefs;
};
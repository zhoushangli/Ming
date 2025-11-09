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

struct SpriteDef
{
	SpriteDef(AABB2 const& uvBounds);
	AABB2 m_uvBounds;
};

class SpriteSheet
{
public:
	SpriteSheet(Texture const& texture, IntVec2 const& gridLayout);
	~SpriteSheet() = default;

    Texture const& GetTexture() const { return m_texture; }

private:
	Texture const&	m_texture;
    std::vector<SpriteDef> m_spriteDefs;
};
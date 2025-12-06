#pragma once

#include "Engine/Core/Vertex.hpp"
#include "Engine/Renderer/Texture.hpp"

enum class TextBoxMode
{
    SHRINK_TO_FIT,
    OVERRUN
};

class BitmapFont
{
    friend class Renderer; // Only the Renderer can create new BitmapFont objects!

private:
    BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture);

public:
    Texture& GetTexture();

    void AddVertsForText2D(
        std::vector<Vertex>& vertexArray, 
        Vec2 textMins,
        float cellHeight, 
        std::string const& text, 
        Rgba8 tint = Rgba8::WHITE, 
        float cellAspectScale = 1.f);

    void AddVertsForTextInBox2D(
        std::vector<Vertex>& verts, 
        std::string const& text, 
        AABB2 const& box, 
        float cellHeight, 
        Rgba8 tint = Rgba8::WHITE,
        float cellAspectScale = 1.f, 
        Vec2 alignment = Vec2(.5f, .5f), 
        TextBoxMode mode = TextBoxMode::SHRINK_TO_FIT, 
        int maxGlyphsToDraw = 99999999);


    float GetTextWidth(float cellHeight, std::string const& text, float cellAspectScale = 1.f);
    float GetTextHeight(float cellHeight, std::string const& text);
    Vec2 GetTextBoundsDimension(float cellHeight, std::string const& text, float cellAspectScale = 1.f);

protected:
    float GetGlyphAspect(int glyphUnicode) const; // For now this will always return m_fontDefaultAspect

protected:
    std::string	m_fontFilePathNameWithNoExtension;
    SpriteSheet	m_fontGlyphsSpriteSheet;
    float		m_fontDefaultAspect = 1.0f; // For basic (tier 1) fonts, set this to the aspect of the sprite sheet texture
};

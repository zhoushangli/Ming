#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Core/String.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <cstdint>
#include <vector>

struct TextLineConfig
{
	float m_ascent  = 0.0f;
	float m_descent = 0.0f;
	float m_lineGap = 0.0f;
};

struct TextLineGlyph
{
	char32_t m_codePoint  = U'\0';
	int      m_glyphIndex = 0;
	float    m_posX       = 0.0f;
	float    m_advance    = 0.0f;
};

struct TextLine
{
	std::vector<TextLineGlyph> m_glyphs;
	TextLineConfig             m_config;
	float                      m_width = 0.0f;
};

struct GlyphImage
{
	std::vector<uint8_t> m_pixels;
	int                  m_width   = 0;
	int                  m_height  = 0;
	int                  m_offsetX = 0;
	int                  m_offsetY = 0;
};

struct GlyphTexture
{
	GlyphImage           m_bitmap;
	Ref<TextureResource> m_texture;
};

struct FontData;

class TextServer
{
public:
	TextServer();
	~TextServer();

	TextServer(TextServer const&)            = delete;
	TextServer& operator=(TextServer const&) = delete;

	// Return a cached glyph, rasterizing and uploading it on the first request.
	// e.g. GetCachedGlyph(fontRID, glyphIndex, 16)
	GlyphTexture const* GetOrCreateGlyphTexture(RID font, int glyphIndex, int fontSize);

	// This function accepts a binary data of .tff file
	RID  CreateFont(std::vector<uint8_t> bytes);
	void FreeFont(RID font);

	int            GetGlyphIndex(RID font, char32_t codePoint) const;
	TextLineConfig GetFontMetrics(RID font, float pixelHeight) const;
	TextLine       GetTextLine(RID font, String const& text, float pixelHeight) const;
	GlyphImage     RasterizeGlyph(RID font, int glyphIndex, float pixelHeight) const;

private:
	RIDOwner<FontData> m_fontOwner;
};

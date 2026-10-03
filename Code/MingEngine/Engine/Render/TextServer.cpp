#include "MingEngine/Engine/Render/TextServer.hpp"

#include "MingEngine/Scene/Resource/TextureResource.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "ThirdParty/stb/stb_truetype.h"

#include <map>
#include <utility>

struct FontData
{
	std::vector<uint8_t> m_bytes;
	stbtt_fontinfo       m_info = {};

	// map<pair<fontSize, glyphIndex>, CachedGlyph>
	std::map<std::pair<int, int>, GlyphTexture> m_glyphCache;
};

TextServer::TextServer()  = default;
TextServer::~TextServer() = default;

GlyphTexture const* TextServer::GetOrCreateGlyphTexture(RID font, int glyphIndex, int fontSize)
{
	FontData* data = m_fontOwner.GetOrNull(font);
	if (data == nullptr || glyphIndex < 0 || fontSize <= 0)
	{
		return nullptr;
	}

	std::pair<int, int> const key(fontSize, glyphIndex);
	auto const                cached = data->m_glyphCache.find(key);
	if (cached != data->m_glyphCache.end())
	{
		return &cached->second;
	}

	GlyphImage           bitmap = RasterizeGlyph(font, glyphIndex, static_cast<float>(fontSize));
	Ref<TextureResource> texture;

	if (bitmap.m_width > 0 && bitmap.m_height > 0)
	{
		std::vector<uint8_t> rgba(bitmap.m_pixels.size() * 4);
		for (size_t i = 0; i < bitmap.m_pixels.size(); ++i)
		{
			rgba[i * 4 + 0] = 255;
			rgba[i * 4 + 1] = 255;
			rgba[i * 4 + 2] = 255;
			rgba[i * 4 + 3] = bitmap.m_pixels[i];
		}

		Ref<Image> image = CreateRef<Image>();
		if (!image->LoadFromRGBA8(IntVec2(bitmap.m_width, bitmap.m_height), std::move(rgba)))
		{
			return nullptr;
		}

		texture = CreateRef<TextureResource>();
		texture->SetName("Glyph");
		texture->m_image = std::move(image);
		if (!texture->InitGPUResources())
		{
			return nullptr;
		}
	}

	data->m_glyphCache[key] = GlyphTexture{ std::move(bitmap), std::move(texture) };
	return &data->m_glyphCache[key];
}

RID TextServer::CreateFont(std::vector<uint8_t> bytes)
{
	if (bytes.empty())
	{
		return RID::Invalid;
	}

	RID       rid  = m_fontOwner.CreateRID();
	FontData* font = m_fontOwner.GetOrNull(rid);
	font->m_bytes  = std::move(bytes);

	int const offset = stbtt_GetFontOffsetForIndex(font->m_bytes.data(), 0);
	if (offset < 0 || !stbtt_InitFont(&font->m_info, font->m_bytes.data(), offset))
	{
		m_fontOwner.Free(rid);
		return RID::Invalid;
	}
	return rid;
}

void TextServer::FreeFont(RID font) { m_fontOwner.Free(font); }

int TextServer::GetGlyphIndex(RID font, char32_t codePoint) const
{
	FontData const* data = m_fontOwner.GetOrNull(font);
	return data != nullptr ? stbtt_FindGlyphIndex(&data->m_info, static_cast<int>(codePoint)) : 0;
}

TextLineConfig TextServer::GetFontMetrics(RID font, float pixelHeight) const
{
	TextLineConfig  metrics;
	FontData const* data = m_fontOwner.GetOrNull(font);
	if (data == nullptr || pixelHeight <= 0.0f)
	{
		return metrics;
	}

	int ascent  = 0;
	int descent = 0;
	int lineGap = 0;
	stbtt_GetFontVMetrics(&data->m_info, &ascent, &descent, &lineGap);
	float const scale = stbtt_ScaleForPixelHeight(&data->m_info, pixelHeight);
	metrics.m_ascent  = ascent * scale;
	metrics.m_descent = descent * scale;
	metrics.m_lineGap = lineGap * scale;
	return metrics;
}

TextLine TextServer::GetTextLine(RID font, String const& text, float pixelHeight) const
{
	TextLine        line;
	FontData const* data = m_fontOwner.GetOrNull(font);
	if (data == nullptr || pixelHeight <= 0.0f)
	{
		return line;
	}

	line.m_config     = GetFontMetrics(font, pixelHeight);
	float const scale = stbtt_ScaleForPixelHeight(&data->m_info, pixelHeight);
	for (uint32_t i = 0; i < text.Length(); ++i)
	{
		char32_t const codePoint = text[i];

		// Currently, we don't support multi-line text
		if (codePoint == U'\n')
		{
			break;
		}

		int const glyphIndex = stbtt_FindGlyphIndex(&data->m_info, static_cast<int>(codePoint));
		if (!line.m_glyphs.empty())
		{
			float const kern =
				stbtt_GetGlyphKernAdvance(&data->m_info, line.m_glyphs.back().m_glyphIndex, glyphIndex) * scale;
			line.m_glyphs.back().m_advance += kern;
			line.m_width += kern;
		}

		int advanceWidth = 0;
		stbtt_GetGlyphHMetrics(&data->m_info, glyphIndex, &advanceWidth, nullptr);
		float const advance = advanceWidth * scale;
		line.m_glyphs.push_back({ codePoint, glyphIndex, line.m_width, advance });
		line.m_width += advance;
	}
	return line;
}

GlyphImage TextServer::RasterizeGlyph(RID font, int glyphIndex, float pixelHeight) const
{
	GlyphImage      bitmap;
	FontData const* data = m_fontOwner.GetOrNull(font);
	if (data == nullptr || glyphIndex < 0 || pixelHeight <= 0.0f)
	{
		return bitmap;
	}

	float const scale = stbtt_ScaleForPixelHeight(&data->m_info, pixelHeight);
	int         x1    = 0;
	int         y1    = 0;
	stbtt_GetGlyphBitmapBox(&data->m_info, glyphIndex, scale, scale, &bitmap.m_offsetX, &bitmap.m_offsetY, &x1, &y1);
	bitmap.m_width  = x1 - bitmap.m_offsetX;
	bitmap.m_height = y1 - bitmap.m_offsetY;
	if (bitmap.m_width <= 0 || bitmap.m_height <= 0)
	{
		return bitmap;
	}

	bitmap.m_pixels.resize(static_cast<size_t>(bitmap.m_width) * bitmap.m_height);
	stbtt_MakeGlyphBitmap(
		&data->m_info,
		bitmap.m_pixels.data(),
		bitmap.m_width,
		bitmap.m_height,
		bitmap.m_width,
		scale,
		scale,
		glyphIndex);

	return bitmap;
}

#include "MingEngine/Engine/Render/TextServer.hpp"

#include "MingEngine/Scene/Resource/TextureResource.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "ThirdParty/stb/stb_truetype.h"

#include <map>
#include <algorithm>
#include <utility>

struct FontData
{
	std::vector<uint8_t> m_bytes;
	stbtt_fontinfo       m_info = {};
	bool m_sizeInEm = false;

	// map<pair<fontSize, glyphIndex>, CachedGlyph>
	std::map<std::pair<int, int>, GlyphTexture> m_glyphCache;
};

namespace
{
float GetFontScale(FontData const& font, float size)
{
	return font.m_sizeInEm ? stbtt_ScaleForMappingEmToPixels(&font.m_info, size)
		: stbtt_ScaleForPixelHeight(&font.m_info, size);
}

void AppendGlyph(FontData const& data, float scale, char32_t codePoint, TextLine& line)
{
	int const glyphIndex = stbtt_FindGlyphIndex(&data.m_info, static_cast<int>(codePoint));
	if (!line.m_glyphs.empty())
	{
		float const kern = stbtt_GetGlyphKernAdvance(&data.m_info, line.m_glyphs.back().m_glyphIndex, glyphIndex) * scale;
		line.m_glyphs.back().m_advance += kern;
		line.m_width += kern;
	}

	int advanceWidth = 0;
	stbtt_GetGlyphHMetrics(&data.m_info, glyphIndex, &advanceWidth, nullptr);
	float const advance = advanceWidth * scale;
	line.m_glyphs.push_back({ codePoint, glyphIndex, line.m_width, advance });
	line.m_width += advance;
}

bool IsWrapSpace(char32_t codePoint)
{
	return codePoint == U' ' || codePoint == U'\t';
}

bool IsCJK(char32_t codePoint)
{
	return (codePoint >= 0x2e80 && codePoint <= 0x9fff)
		|| (codePoint >= 0xf900 && codePoint <= 0xfaff)
		|| (codePoint >= 0x20000 && codePoint <= 0x323af);
}
}

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

RID TextServer::CreateFont(std::vector<uint8_t> bytes, bool sizeInEm)
{
	if (bytes.empty())
	{
		return RID::Invalid;
	}

	RID       rid  = m_fontOwner.CreateRID();
	FontData* font = m_fontOwner.GetOrNull(rid);
	font->m_bytes  = std::move(bytes);
	font->m_sizeInEm = sizeInEm;

	int const offset = stbtt_GetFontOffsetForIndex(font->m_bytes.data(), 0);
	if (offset < 0 || !stbtt_InitFont(&font->m_info, font->m_bytes.data(), offset))
	{
		m_fontOwner.Free(rid);
		return RID::Invalid;
	}

	return rid;
}

void TextServer::FreeFont(RID font)
{
	m_fontOwner.Free(font);
}

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
	float const scale = GetFontScale(*data, pixelHeight);
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
	float const scale = GetFontScale(*data, pixelHeight);
	for (uint32_t i = 0; i < text.Length(); ++i)
	{
		char32_t const codePoint = text[i];

		// Keep the single-line entry point unchanged for editable fields.
		// e.g. GetTextLine(font, "first\nsecond", 16) measures only "first".
		if (codePoint == U'\n')
		{
			break;
		}

		AppendGlyph(*data, scale, codePoint, line);
	}

	return line;
}

std::vector<TextLine> TextServer::GetTextLines(
	RID font, String const& text, float pixelHeight, float width, AutowrapMode wrap) const
{
	std::vector<TextLine> lines;
	FontData const* data = m_fontOwner.GetOrNull(font);
	if (data == nullptr || pixelHeight <= 0.f)
	{
		return lines;
	}

	float const scale = GetFontScale(*data, pixelHeight);
	uint32_t paragraphStart = 0;
	while (paragraphStart <= text.Length())
	{
		uint32_t paragraphEnd = paragraphStart;
		while (paragraphEnd < text.Length() && text[paragraphEnd] != U'\n' && text[paragraphEnd] != U'\r')
		{
			++paragraphEnd;
		}

		if (wrap == AutowrapMode::Off || width <= 0.f || paragraphStart == paragraphEnd)
		{
			lines.push_back(GetTextLine(font, text.Substr(paragraphStart, paragraphEnd - paragraphStart), pixelHeight));
		}
		else
		{
			uint32_t start = paragraphStart;
			while (start < paragraphEnd)
			{
				TextLine candidate;
				uint32_t end = start;
				uint32_t breakEnd = start;
				bool hasText = false;
				while (end < paragraphEnd)
				{
					char32_t const cp = text[end];
					if (end > start && IsCJK(cp))
					{
						breakEnd = end;
					}

					if (IsWrapSpace(cp) && hasText && (end == start || !IsWrapSpace(text[end - 1])))
					{
						breakEnd = end;
					}

					AppendGlyph(*data, scale, cp, candidate);
					if (candidate.m_width > width && end > start)
					{
						break;
					}

					hasText = hasText || !IsWrapSpace(cp);
					++end;
					if (IsCJK(cp))
					{
						breakEnd = end;
					}
				}

				uint32_t const cut = end < paragraphEnd && breakEnd > start ? breakEnd : end;
				lines.push_back(GetTextLine(font, text.Substr(start, cut - start), pixelHeight));
				start = cut;
				if (start < paragraphEnd)
				{
					while (start < paragraphEnd && IsWrapSpace(text[start]))
					{
						++start;
					}
				}
			}
		}

		if (paragraphEnd == text.Length())
		{
			break;
		}

		paragraphStart = paragraphEnd + 1;
		if (text[paragraphEnd] == U'\r' && paragraphStart < text.Length() && text[paragraphStart] == U'\n')
		{
			++paragraphStart;
		}
	}

	return lines;
}

TextLine TextServer::TrimTextLine(
	RID font, TextLine const& line, float pixelHeight, float width, TextOverrunBehavior behavior) const
{
	if (behavior == TextOverrunBehavior::TrimNothing || line.m_width <= width)
	{
		return line;
	}

	TextLine result;
	result.m_config = line.m_config;
	FontData const* data = m_fontOwner.GetOrNull(font);
	if (data == nullptr || pixelHeight <= 0.f || width <= 0.f)
	{
		return result;
	}

	String const suffix = behavior == TextOverrunBehavior::EllipsisAlways
		? (GetGlyphIndex(font, U'\u2026') != 0 ? String(U"\u2026") : String("...")) : String();
	TextLine const ellipsis = GetTextLine(font, suffix, pixelHeight);
	if (ellipsis.m_width > width)
	{
		return result;
	}

	// 1) Find the longest prefix, including its kerning against the ellipsis.
	float const scale = GetFontScale(*data, pixelHeight);
	TextLine prefix;
	size_t keep = 0;
	for (size_t i = 0; i < line.m_glyphs.size(); ++i)
	{
		AppendGlyph(*data, scale, line.m_glyphs[i].m_codePoint, prefix);
		float const kern = ellipsis.m_glyphs.empty() ? 0.f
			: stbtt_GetGlyphKernAdvance(&data->m_info, prefix.m_glyphs.back().m_glyphIndex,
				ellipsis.m_glyphs.front().m_glyphIndex) * scale;
		if (prefix.m_width + kern + ellipsis.m_width <= width)
		{
			keep = i + 1;
		}
	}

	// 2) Shape the retained prefix and suffix together so positions and measurement agree.
	for (size_t i = 0; i < keep; ++i)
	{
		AppendGlyph(*data, scale, line.m_glyphs[i].m_codePoint, result);
	}

	for (uint32_t i = 0; i < suffix.Length(); ++i)
	{
		AppendGlyph(*data, scale, suffix[i], result);
	}

	return result;
}

GlyphImage TextServer::RasterizeGlyph(RID font, int glyphIndex, float pixelHeight) const
{
	GlyphImage      bitmap;
	FontData const* data = m_fontOwner.GetOrNull(font);
	if (data == nullptr || glyphIndex < 0 || pixelHeight <= 0.0f)
	{
		return bitmap;
	}

	float const scale = GetFontScale(*data, pixelHeight);
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

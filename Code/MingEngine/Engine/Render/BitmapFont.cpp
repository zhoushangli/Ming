#include "MingEngine/Engine/Render/BitmapFont.hpp"

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Engine/Render/Texture.hpp"

#include <string>
#include <vector>

using namespace Math;

int const kFontSpriteSheetCols = 16;
int const kFontSpriteSheetRows = 16;

BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture)
	: m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension),
	  m_fontGlyphsSpriteSheet(&fontTexture, IntVec2(kFontSpriteSheetCols, kFontSpriteSheetRows))
{
	IntVec2 texDims     = fontTexture.GetDimensions();
	float   texAspect   = (texDims.y == 0) ? 1.f : (float)texDims.x / (float)texDims.y;
	float   gridAspect  = (float)kFontSpriteSheetCols / (float)kFontSpriteSheetRows;
	m_fontDefaultAspect = texAspect / gridAspect;
}

Texture* BitmapFont::GetTexture() { return m_fontGlyphsSpriteSheet.GetTexture(); }

void BitmapFont::AddVertsForText2D(
	std::vector<Vertex>& verts,
	Vec2                 textMins,
	float                cellHeight,
	std::string const&   text,
	Rgba8                tint,
	float                cellAspectScale)
{
	float cellWidth  = cellHeight * m_fontDefaultAspect * cellAspectScale;
	Vec2  pen        = textMins;
	float lineStartX = textMins.x;

	for (char c : text)
	{
		if (c == '\n')
		{
			pen.x = lineStartX;
			pen.y -= cellHeight;
			continue;
		}

		int   index = (int)c;
		AABB2 uv    = m_fontGlyphsSpriteSheet.GetSpriteUVs(index);
		AABB2 bounds(pen, pen + Vec2(cellWidth, cellHeight));

		verts.emplace_back(Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.f), tint, uv.m_mins);
		verts.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_mins.y, 0.f), tint, Vec2(uv.m_maxs.x, uv.m_mins.y));
		verts.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.f), tint, uv.m_maxs);

		verts.emplace_back(Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.f), tint, uv.m_mins);
		verts.emplace_back(Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.f), tint, uv.m_maxs);
		verts.emplace_back(Vec3(bounds.m_mins.x, bounds.m_maxs.y, 0.f), tint, Vec2(uv.m_mins.x, uv.m_maxs.y));

		pen.x += cellWidth;
	}
}

void BitmapFont::AddVertsForTextInBox2D(
	std::vector<Vertex>& verts,
	std::string const&   text,
	AABB2 const&         box,
	float                cellHeight,
	Rgba8                tint,
	float                cellAspectScale,
	Vec2                 alignment,
	TextBoxMode          mode,
	int                  maxGlyphsToDraw)
{
	Vec2  textBoundsDimension = GetTextBoundsDimension(cellHeight, text, cellAspectScale);
	AABB2 textBounds          = AABB2(box.m_mins, box.m_mins + textBoundsDimension);

	Vec2 gaps = box.GetDimensions() - textBounds.GetDimensions();

	if (mode == TextBoxMode::SHRINK_TO_FIT)
	{
		Vec2 dims    = textBounds.GetDimensions();
		Vec2 boxDims = box.GetDimensions();

		float sx    = boxDims.x / dims.x;
		float sy    = boxDims.y / dims.y;
		float scale = Min(sx, sy);

		if (scale < 1.f)
		{
			cellHeight *= scale;

			textBoundsDimension = GetTextBoundsDimension(cellHeight, text, cellAspectScale);
			textBounds          = AABB2(box.m_mins, box.m_mins + textBoundsDimension);

			gaps = box.GetDimensions() - textBounds.GetDimensions();
		}
	}

	Vec2  start           = box.m_mins + gaps * alignment;
	float baselineYOffset = textBoundsDimension.y - cellHeight;
	start.y += baselineYOffset;
	int         glyphCount = Min((int)text.size(), maxGlyphsToDraw);
	std::string clipped    = text.substr(0, glyphCount + 1);

	AddVertsForText2D(verts, start, cellHeight, clipped, tint, cellAspectScale);
}

void BitmapFont::AddVertsForText3DAtOriginXForward(
	std::vector<Vertex>& verts,
	float                cellHeight,
	std::string const&   text,
	Rgba8 const&         tint /*= Rgba8::kWhite*/,
	float                cellAspect /*= 1.0f*/,
	Vec2 const&          alignment /*= Vec2(0.5f, 0.5f)*/,
	int                  maxGlyphsToDraw /*= 999*/)
{
	int glyphCount = Min((int)text.size(), maxGlyphsToDraw);
	if (glyphCount <= 0 || cellHeight <= 0.f)
	{
		return;
	}

	std::string clippedText    = text.substr(0, glyphCount);
	Vec2        textDimensions = GetTextBoundsDimension(cellHeight, clippedText, cellAspect);
	Vec2        textMins       = -textDimensions * alignment;

	std::vector<Vertex> textVerts;
	AddVertsForText2D(textVerts, textMins, cellHeight, clippedText, tint, cellAspect);

	Matrix4x4 textTransform(0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f);
	TransformVertexArray3D(textVerts, textTransform);

	verts.insert(verts.end(), textVerts.begin(), textVerts.end());
}

float BitmapFont::GetTextWidth(float cellHeight, std::string const& text, float cellAspectScale)
{
	float cellWidth = cellHeight * m_fontDefaultAspect * cellAspectScale;

	int maxLen = 0;
	int curLen = 0;

	for (char c : text)
	{
		if (c == '\n')
		{
			maxLen = Max(maxLen, curLen);
			curLen = 0;
		}
		else
		{
			++curLen;
		}
	}

	maxLen = Max(maxLen, curLen);
	return cellWidth * (float)maxLen;
}

float BitmapFont::GetTextHeight(float cellHeight, std::string const& text)
{
	int lines = 1;
	for (char c : text)
	{
		if (c == '\n')
		{
			++lines;
		}
	}

	return cellHeight * (float)lines;
}

Vec2 BitmapFont::GetTextBoundsDimension(float cellHeight, std::string const& text, float cellAspectScale /*= 1.f*/)
{
	float textWidth  = GetTextWidth(cellHeight, text, cellAspectScale);
	float textHeight = GetTextHeight(cellHeight, text);
	return Vec2(textWidth, textHeight);
}

float BitmapFont::GetGlyphAspect(int) const { return m_fontDefaultAspect; }

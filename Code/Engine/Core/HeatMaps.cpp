#include "Engine/Core/HeatMaps.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/MathUtils.hpp"

TileHeatMap::TileHeatMap(IntVec2 const& dimensions) : m_dimensions(dimensions)
{
    int numTiles = m_dimensions.x * m_dimensions.y;
    m_values = new float[numTiles];
    SetAllValues(0.0f);
}

TileHeatMap::~TileHeatMap()
{
    delete[] m_values;
    m_values = nullptr;
}

void TileHeatMap::SetAllValues(float value)
{
    int numTiles = m_dimensions.x * m_dimensions.y;
    for (int i = 0; i < numTiles; ++i)
    {
        m_values[i] = value;
    }
}

int TileHeatMap::GetIndexForCoords(int x, int y) const
{
    return y * m_dimensions.x + x;
}

int TileHeatMap::GetIndexForCoords(IntVec2 const& coords) const
{
    return GetIndexForCoords(coords.x, coords.y);
}

float TileHeatMap::GetValue(int x, int y) const
{
    int idx = GetIndexForCoords(x, y);
    return m_values[idx];
}

void TileHeatMap::SetValue(int x, int y, float value)
{
    int idx = GetIndexForCoords(x, y);
    m_values[idx] = value;
}

void TileHeatMap::AddValue(int x, int y, float value)
{
    int idx = GetIndexForCoords(x, y);
    m_values[idx] += value;
}

float TileHeatMap::GetValue(IntVec2 const& coords) const
{
    return GetValue(coords.x, coords.y);
}

void TileHeatMap::SetValue(IntVec2 const& coords, float value)
{
    return SetValue(coords.x, coords.y, value);
}

void TileHeatMap::AddValue(IntVec2 const& coords, float value)
{
    return AddValue(coords.x, coords.y, value);
}

float TileHeatMap::GetMinValue() const
{
    float minValue = 1e9f;

    for (int y = 0; y < m_dimensions.y; ++y)
    {
        for (int x = 0; x < m_dimensions.x; ++x)
        {
            IntVec2 coords(x, y);
            float value = GetValue(coords);
            if (value < minValue && value != TILE_HEAT_MAP_INVALID_VALUE)
            {
                minValue = value;
            }
        }
    }

    return minValue;
}

float TileHeatMap::GetMaxValue() const
{
    float maxValue = -1e9f;

    for (int y = 0; y < m_dimensions.y; ++y)
    {
        for (int x = 0; x < m_dimensions.x; ++x)
        {
            IntVec2 coords(x, y);
            float value = GetValue(coords);
            if (value > maxValue && value != TILE_HEAT_MAP_INVALID_VALUE)
            {
                maxValue = value;
            }
        }
    }

    return maxValue;
}

void TileHeatMap::AddVertsForDebugDraw(
    std::vector<Vertex>& verts,
    AABB2 totalBounds,
    FloatRange valueRange,
    Rgba8 lowColor,
    Rgba8 highColor,
    float specialValue,
    Rgba8 specialColor) const
{
    int numTilesX = m_dimensions.x;
    int numTilesY = m_dimensions.y;
    Vec2 tileSize = totalBounds.GetDimensions();
    tileSize.x /= (float)numTilesX;
    tileSize.y /= (float)numTilesY;

    for (int y = 0; y < numTilesY; ++y)
    {
        for (int x = 0; x < numTilesX; ++x)
        {
            IntVec2 coords(x, y);
            float value = GetValue(coords);

            Vec2 mins = totalBounds.m_mins + Vec2((float)x * tileSize.x, (float)y * tileSize.y);
            Vec2 maxs = mins + tileSize;
            AABB2 tileBounds(mins, maxs);

            Rgba8 color;
            if (value == specialValue)
            {
                color = specialColor;
            }
            else
            {
                value          = GetClamped(value, valueRange.m_min, valueRange.m_max);
                float fraction = (value - valueRange.m_min) / (valueRange.m_max - valueRange.m_min);
                color          = Interpolate(lowColor, highColor, fraction);
            }

            verts.emplace_back(Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, 0.f), color);
            verts.emplace_back(Vec3(tileBounds.m_maxs.x, tileBounds.m_mins.y, 0.f), color);
            verts.emplace_back(Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, 0.f), color);

            verts.emplace_back(Vec3(tileBounds.m_mins.x, tileBounds.m_mins.y, 0.f), color);
            verts.emplace_back(Vec3(tileBounds.m_maxs.x, tileBounds.m_maxs.y, 0.f), color);
            verts.emplace_back(Vec3(tileBounds.m_mins.x, tileBounds.m_maxs.y, 0.f), color);
        }
    }
}
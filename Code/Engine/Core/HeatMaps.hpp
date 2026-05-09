#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"

#include <vector>

float const kTileHeatMapInvalidValue = 1e9f;

struct Vertex;

class TileHeatMap
{
public:
    explicit TileHeatMap(IntVec2 const& dimensions);
    ~TileHeatMap();

    void SetAllValues(float value);

    float GetValue(int x, int y) const;
    void SetValue(int x, int y, float value);
    void AddValue(int x, int y, float value);

    float GetValue(IntVec2 const& coords) const;
    void SetValue(IntVec2 const& coords, float value);
    void AddValue(IntVec2 const& coords, float value);

    float GetMinValue() const;
    float GetMaxValue() const;

    void AddVertsForDebugDraw(
        std::vector<Vertex>& verts,
        AABB2 totalBounds     = AABB2::kUnit,
        FloatRange valueRange = FloatRange::kZeroToOne,
        Rgba8 lowColor        = Rgba8::kBlack,
        Rgba8 highColor       = Rgba8::kWhite,
        float specialValue    = -1,
        Rgba8 specialColor    = Rgba8::kBlack) const;

    IntVec2 GetDimensions() const { return m_dimensions; }

    void GeneratePath(std::vector<Vec2>& path, Vec2 position);

private:
    int GetIndexForCoords(int x, int y) const;
    int GetIndexForCoords(IntVec2 const& coords) const;

private:
    IntVec2 m_dimensions;
    float* m_values = nullptr;
};

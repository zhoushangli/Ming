#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"

#include <vector>

struct Vertex;

class TileHeatMap
{
public:
    explicit TileHeatMap(IntVec2 const& dimensions);
    ~TileHeatMap();

    void SetAllValues(float value);
    float GetValue(IntVec2 const& coords) const;
    void SetValue(IntVec2 const& coords, float value);
    void AddValue(IntVec2 const& coords, float value);

    void AddVertsForDebugDraw(
        std::vector<Vertex>& verts,
        AABB2 totalBounds     = AABB2::UNIT,
        FloatRange valueRange = FloatRange::ZERO_TO_ONE,
        Rgba8 lowColor        = Rgba8::BLACK,
        Rgba8 highColor       = Rgba8::WHITE,
        float specialValue    = -1,
        Rgba8 specialColor    = Rgba8::BLACK) const;

    IntVec2 GetDimensions() const { return m_dimensions; }

private:
    int GetIndexForCoords(IntVec2 const& coords) const;

private:
    IntVec2 m_dimensions;
    float* m_values = nullptr;
};

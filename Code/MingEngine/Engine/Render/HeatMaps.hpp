#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/FloatRange.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"

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
	void  SetValue(int x, int y, float value);
	void  AddValue(int x, int y, float value);

	float GetValue(IntVec2 const& coords) const;
	void  SetValue(IntVec2 const& coords, float value);
	void  AddValue(IntVec2 const& coords, float value);

	float GetMinValue() const;
	float GetMaxValue() const;

	void AddVertsForDebugDraw(
		std::vector<Vertex>& verts,
		AABB2                totalBounds  = AABB2::Unit,
		FloatRange           valueRange   = FloatRange::ZeroToOne,
		Color                lowColor     = Color::Black,
		Color                highColor    = Color::White,
		float                specialValue = -1,
		Color                specialColor = Color::Black) const;

	IntVec2 GetDimensions() const { return m_dimensions; }

	void GeneratePath(std::vector<Vector2>& path, Vector2 position);

private:
	int GetIndexForCoords(int x, int y) const;
	int GetIndexForCoords(IntVec2 const& coords) const;

private:
	IntVec2 m_dimensions;
	float*  m_values = nullptr;
};

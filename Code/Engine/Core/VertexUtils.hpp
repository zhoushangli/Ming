#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/LineSegment2.hpp"

#include <vector>

struct Rgba8;
struct Vertex;
struct Vec2;
class AABB2;

void TransformVertexArrayXY3D(
    int numVerts,
    Vertex* verts,
    float scaleXY,
    float rotationDegreesAboutZ,
    Vec2 const& translationXY
);

void AddVertsForAABB2D(std::vector<Vertex>& testTextureVerts, const AABB2& texturedAABB2, Rgba8 color);
void AddVertsForDisc2D(std::vector<Vertex>& verts, Vec2 discCenter, float discRadius, Rgba8 color);
void AddVertsForRing2D(std::vector<Vertex>& verts, Vec2 ringCenter, float ringRadius, float thickness, Rgba8 color);
void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Rgba8 color);
void AddVertsForOBB2D(std::vector<Vertex>& verts, OBB2 const& orientedBox, Rgba8 color);
void AddVertsForSector2D(std::vector<Vertex>& verts, Vec2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius, Rgba8 color);
void AddVertsForCapsule2D(std::vector<Vertex>& verts, Vec2 boneStart, Vec2 boneEnd, float radius, Rgba8 color);
void AddVertsForTriangle2D(std::vector<Vertex>& verts, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2, Rgba8 color); // Counter-Clockwise
void AddVertsForLineSegment2D(std::vector<Vertex>& verts, Vec2 start, Vec2 end, Vec2 thickness, Rgba8 color);
void AddVertsForInfiniteLine2D(std::vector<Vertex>& verts, Vec2 pointOnLine, Vec2 anotherPointOnLine, float thickness, Rgba8 color);
void AddVertsForDisc2D(std::vector<Vertex>& verts, Disc2 const& disc, Rgba8 color); // Also maybe num sides

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Capsule2 const& capsule, Rgba8 color);
void AddVertsForTriangle2D(std::vector<Vertex>& verts, Triangle2 const& triangle, Rgba8 color);
void AddVertsForLineSegment2D(std::vector<Vertex>& verts, LineSegment2 const& lineSegment, float thickness, Rgba8 color);
void AddVertsForInfiniteLine2D(std::vector<Vertex>& verts, LineSegment2 const& infiniteLine, float thickness, Rgba8 color);
#pragma once

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

void AddVertsForAABB2D(
    std::vector<Vertex>& testTextureVerts, 
    const AABB2& texturedAABB2, 
    const Rgba8& color
);

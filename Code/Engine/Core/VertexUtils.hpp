#pragma once

#include "Engine/Core/Vertex.hpp"

#include "Engine/Math/Vec2.hpp"

void TransformVertexArrayXY3D(
    int numVerts,
    Vertex* verts,
    float scaleXY,
    float rotationDegreesAboutZ,
    Vec2 const& translationXY
);

void AddVertsForAABB2();

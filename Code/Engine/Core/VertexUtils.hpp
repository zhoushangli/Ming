#pragma once

#include <Engine/Math/Vec2.hpp>
#include "Vertex.hpp"

void TransformVertexArrayXY3D(
    int numVerts,
    Vertex* verts,
    float scaleXY,
    float rotationDegreesAboutZ,
    Vec2 const& translationXY
);

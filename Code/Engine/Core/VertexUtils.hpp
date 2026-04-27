#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Capsule3.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Math/LineSegment2.hpp"

#include <vector>

struct Vertex;
struct Vec2;
class AABB2;

#pragma region 2D

AABB2 GetVertexBounds2D(const std::vector<Vertex> &verts);

void AddVertsForDisc2D(std::vector<Vertex> &verts, Vec2 discCenter, float discRadius, Rgba8 color = Rgba8::WHITE);
void AddVertsForRing2D(std::vector<Vertex> &verts, Vec2 ringCenter, float ringRadius, float thickness = 0.1f, Rgba8 color = Rgba8::WHITE);
void AddVertsForAABB2D(std::vector<Vertex> &verts, AABB2 const &alignedBox, Rgba8 color = Rgba8::WHITE);
void AddVertsForAABB2D(std::vector<Vertex> &verts, AABB2 const &alignedBox, Rgba8 color, Vec2 uvAtMins, Vec2 uvAtMaxs);
void AddVertsForOBB2D(std::vector<Vertex> &verts, OBB2 const &orientedBox, Rgba8 color = Rgba8::WHITE);
void AddVertsForSector2D(std::vector<Vertex> &verts, Vec2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius, Rgba8 color = Rgba8::WHITE);
void AddVertsForCapsule2D(std::vector<Vertex> &verts, Vec2 boneStart, Vec2 boneEnd, float radius, Rgba8 color = Rgba8::WHITE);
void AddVertsForTriangle2D(std::vector<Vertex> &verts, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2, Rgba8 color = Rgba8::WHITE); // Counter-Clockwise
void AddVertsForLineSegment2D(std::vector<Vertex> &verts, Vec2 start, Vec2 end, Vec2 thickness = Vec2(1.f, 1.f), Rgba8 color = Rgba8::WHITE);
void AddVertsForInfiniteLine2D(std::vector<Vertex> &verts, Vec2 pointOnLine, Vec2 anotherPointOnLine, float thickness = 0.1f, Rgba8 color = Rgba8::WHITE);
void AddVertsForDisc2D(std::vector<Vertex> &verts, Disc2 const &disc, Rgba8 color = Rgba8::WHITE);

void AddVertsForCapsule2D(std::vector<Vertex> &verts, Capsule2 const &capsule, Rgba8 color = Rgba8::WHITE);
void AddVertsForTriangle2D(std::vector<Vertex> &verts, Triangle2 const &triangle, Rgba8 color = Rgba8::WHITE);
void AddVertsForLineSegment2D(std::vector<Vertex> &verts, LineSegment2 const &lineSegment, float thickness = 0.1f, Rgba8 color = Rgba8::WHITE);
void AddVertsForInfiniteLine2D(std::vector<Vertex> &verts, LineSegment2 const &infiniteLine, float thickness = 0.1f, Rgba8 color = Rgba8::WHITE);
void AddVertsForArrow2D(std::vector<Vertex> &verts, Vec2 tailPos, Vec2 tipPos, float arrowSize = 1.f, float lineThickness = 0.1f, Rgba8 color = Rgba8::WHITE);

#pragma endregion

#pragma region 3D

void TransformVertexArrayXY3D(
    int numVerts,
    Vertex *verts,
    float scaleXY,
    float rotationDegreesAboutZ,
    Vec2 const &translationXY);
void TransformVertexArray3D(std::vector<Vertex> &verts, const Matrix4x4 &transform);

void AddVertsForQuad3D(
    std::vector<Vertex> &verts,
    const Vec3 &bottomLeft,
    const Vec3 &bottomRight,
    const Vec3 &topRight,
    const Vec3 &topLeft,
    const Rgba8 &color = Rgba8::WHITE,
    const AABB2 &UVs = AABB2::UNIT);

void AddVertsForQuad3D(
    std::vector<Vertex> &verts,
    std::vector<unsigned int> &indexes,
    const Vec3 &bottomLeft,
    const Vec3 &bottomRight,
    const Vec3 &topRight,
    const Vec3 &topLeft,
    const Rgba8 &color = Rgba8::WHITE,
    const AABB2 &UVs = AABB2::UNIT);

void AddVertsForAABB3D(std::vector<Vertex> &verts,
                       const AABB3 &bounds,
                       const Rgba8 &color = Rgba8::WHITE,
                       const AABB2 &UVs = AABB2::UNIT);

void AddVertsForAABB3D(std::vector<Vertex> &verts,
                       std::vector<unsigned int> &indexes,
                       const AABB3 &bounds,
                       const Rgba8 &color = Rgba8::WHITE,
                       const AABB2 &UVs = AABB2::UNIT);

void AddVertsForSphere3D(std::vector<Vertex> &verts,
                         const Vec3 &center,
                         float radius,
                         const Rgba8 &color = Rgba8::WHITE,
                         const AABB2 &UVs = AABB2::UNIT, int numSlices = 32, int numStacks = 16);

void AddVertsForCylinder3D(std::vector<Vertex> &verts,
                           const Vec3 &start,
                           const Vec3 &end,
                           float radius,
                           const Rgba8 &color = Rgba8::WHITE,
                           const AABB2 &UVs = AABB2::UNIT,
                           int numSlices = 32);

void AddVertsForCapsule3D(std::vector<Vertex> &verts,
                          const Vec3 &start,
                          const Vec3 &end,
                          float radius,
                          const Rgba8 &color = Rgba8::WHITE,
                          int numSlices = 32,
                          int numStacks = 16);

void AddVertsForCapsule3D(std::vector<Vertex> &verts,
                          Capsule3 const &capsule,
                          const Rgba8 &color = Rgba8::WHITE,
                          int numSlices = 32,
                          int numStacks = 16);

void AddVertsForCone3D(std::vector<Vertex> &verts,
                       const Vec3 &start,
                       const Vec3 &end,
                       float radius,
                       const Rgba8 &color = Rgba8::WHITE,
                       const AABB2 &UVs = AABB2::UNIT,
                       int numSlices = 32);

void AddVertsForArrow3D(std::vector<Vertex> &verts,
                        Vec3 const &start,
                        Vec3 const &end,
                        float radius,
                        Rgba8 const &color = Rgba8::WHITE,
                        int numSlices = 32);

#pragma endregion


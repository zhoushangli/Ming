#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/Capsule2.hpp"
#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/Disc2.hpp"
#include "MingEngine/Core/Math/LineSegment2.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/OBB2.hpp"
#include "MingEngine/Core/Math/Triangle2.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"

#include <vector>

struct Vertex;
struct Vec2;
class AABB2;

#pragma region 2D

AABB2 GetVertexBounds2D(const std::vector<Vertex>& verts);

void AddVertsForDisc2D(std::vector<Vertex>& verts, Vec2 discCenter, float discRadius, Color color = Color::White);
void AddVertsForRing2D(
	std::vector<Vertex>& verts, Vec2 ringCenter, float ringRadius, float thickness = 0.1f, Color color = Color::White);
void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Color color = Color::White);
void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Color color, Vec2 uvAtMins, Vec2 uvAtMaxs);
void AddVertsForOBB2D(std::vector<Vertex>& verts, OBB2 const& orientedBox, Color color = Color::White);
void AddVertsForSector2D(
	std::vector<Vertex>& verts,
	Vec2                 sectorOrigin,
	float                sectorForwardDegrees,
	float                sectorApertureDegrees,
	float                sectorRadius,
	Color                color = Color::White);
void AddVertsForCapsule2D(
	std::vector<Vertex>& verts, Vec2 boneStart, Vec2 boneEnd, float radius, Color color = Color::White);
void AddVertsForTriangle2D(
	std::vector<Vertex>& verts, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2, Color color = Color::White); // Counter-Clockwise
void AddVertsForLineSegment2D(
	std::vector<Vertex>& verts, Vec2 start, Vec2 end, Vec2 thickness = Vec2(1.f, 1.f), Color color = Color::White);
void AddVertsForInfiniteLine2D(
	std::vector<Vertex>& verts,
	Vec2                 pointOnLine,
	Vec2                 anotherPointOnLine,
	float                thickness = 0.1f,
	Color                color     = Color::White);
void AddVertsForDisc2D(std::vector<Vertex>& verts, Disc2 const& disc, Color color = Color::White);

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Capsule2 const& capsule, Color color = Color::White);
void AddVertsForTriangle2D(std::vector<Vertex>& verts, Triangle2 const& triangle, Color color = Color::White);
void AddVertsForLineSegment2D(
	std::vector<Vertex>& verts, LineSegment2 const& lineSegment, float thickness = 0.1f, Color color = Color::White);
void AddVertsForInfiniteLine2D(
	std::vector<Vertex>& verts, LineSegment2 const& infiniteLine, float thickness = 0.1f, Color color = Color::White);
void AddVertsForArrow2D(
	std::vector<Vertex>& verts,
	Vec2                 tailPos,
	Vec2                 tipPos,
	float                arrowSize     = 1.f,
	float                lineThickness = 0.1f,
	Color                color         = Color::White);

#pragma endregion

#pragma region 3D

AABB3 GetVertexBounds3D(Vertex const* vertices, size_t numVerts);
AABB3 GetVertexBounds3D(std::vector<Vertex> const& vertices);

void TransformVertexArrayXY3D(
	int numVerts, Vertex* verts, float scaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY);
void TransformVertexArray3D(std::vector<Vertex>& verts, const Matrix4x4& transform);

void AddVertsForQuad3D(
	std::vector<Vertex>& verts,
	const Vec3&          bottomLeft,
	const Vec3&          bottomRight,
	const Vec3&          topRight,
	const Vec3&          topLeft,
	const Color&         color = Color::White,
	const AABB2&         UVs   = AABB2::Unit);

void AddVertsForQuad3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vec3&                bottomLeft,
	const Vec3&                bottomRight,
	const Vec3&                topRight,
	const Vec3&                topLeft,
	const Color&               color = Color::White,
	const AABB2&               UVs   = AABB2::Unit);

void AddVertsForAABB3D(
	std::vector<Vertex>& verts, const AABB3& bounds, const Color& color = Color::White, const AABB2& UVs = AABB2::Unit);

void AddVertsForAABB3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const AABB3&               bounds,
	const Color&               color = Color::White,
	const AABB2&               UVs   = AABB2::Unit);

void AddVertsForSphere3D(
	std::vector<Vertex>& verts,
	const Vec3&          center,
	float                radius,
	const Color&         color     = Color::White,
	const AABB2&         UVs       = AABB2::Unit,
	int                  numSlices = 16,
	int                  numStacks = 8);

void AddVertsForSphere3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vec3&                center,
	float                      radius,
	const Color&               color     = Color::White,
	const AABB2&               UVs       = AABB2::Unit,
	int                        numSlices = 16,
	int                        numStacks = 8);

void AddVertsForCylinder3D(
	std::vector<Vertex>& verts,
	const Vec3&          start,
	const Vec3&          end,
	float                radius,
	const Color&         color     = Color::White,
	const AABB2&         UVs       = AABB2::Unit,
	int                  numSlices = 16);

void AddVertsForCylinder3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vec3&                start,
	const Vec3&                end,
	float                      radius,
	const Color&               color     = Color::White,
	const AABB2&               UVs       = AABB2::Unit,
	int                        numSlices = 16);

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	const Vec3&          start,
	const Vec3&          end,
	float                radius,
	const AABB2&         UVs,
	const Color&         color     = Color::White,
	int                  numSlices = 16,
	int                  numStacks = 8);

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	const Vec3&          start,
	const Vec3&          end,
	float                radius,
	const Color&         color     = Color::White,
	int                  numSlices = 16,
	int                  numStacks = 8);

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Capsule3 const&      capsule,
	const AABB2&         UVs,
	const Color&         color     = Color::White,
	int                  numSlices = 16,
	int                  numStacks = 8);

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Capsule3 const&      capsule,
	const Color&         color     = Color::White,
	int                  numSlices = 16,
	int                  numStacks = 8);

void AddVertsForCone3D(
	std::vector<Vertex>& verts,
	const Vec3&          start,
	const Vec3&          end,
	float                radius,
	const Color&         color     = Color::White,
	const AABB2&         UVs       = AABB2::Unit,
	int                  numSlices = 16);

void AddVertsForCone3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vec3&                start,
	const Vec3&                end,
	float                      radius,
	const Color&               color     = Color::White,
	const AABB2&               UVs       = AABB2::Unit,
	int                        numSlices = 16);

void AddVertsForArrow3D(
	std::vector<Vertex>& verts,
	Vec3 const&          start,
	Vec3 const&          end,
	float                radius,
	Color const&         color     = Color::White,
	int                  numSlices = 16);

#pragma endregion

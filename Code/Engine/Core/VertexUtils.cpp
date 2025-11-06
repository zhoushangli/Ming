#include "Engine/Core/VertexUtils.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/MathUtils.hpp"

void TransformVertexArrayXY3D(int numVerts, Vertex* verts, float scaleXY, 
	float rotationDegreesAboutZ, Vec2 const& translationXY)
{
	Vec2 iBasis = Vec2::MakeFromPolarDegrees(rotationDegreesAboutZ, scaleXY);
	Vec2 jBasis = iBasis.GetRotatedBy90Degrees();
	for (int index = 0; index < numVerts; ++index)
	{
		Vec3& position = verts[index].m_position;
		TransformPositionXY3D(position, iBasis, jBasis, translationXY);
	}
}

void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Rgba8 color)
{
    Vec2 mins = alignedBox.m_mins;
    Vec2 maxs = alignedBox.m_maxs;

    verts.emplace_back(Vec3(mins.x, mins.y, 0.f), color, Vec2(0.f, 0.f));
    verts.emplace_back(Vec3(maxs.x, mins.y, 0.f), color, Vec2(1.f, 0.f));
    verts.emplace_back(Vec3(maxs.x, maxs.y, 0.f), color, Vec2(1.f, 1.f));

    verts.emplace_back(Vec3(mins.x, mins.y, 0.f), color, Vec2(0.f, 0.f));
    verts.emplace_back(Vec3(maxs.x, maxs.y, 0.f), color, Vec2(1.f, 1.f));
    verts.emplace_back(Vec3(mins.x, maxs.y, 0.f), color, Vec2(0.f, 1.f));
}

void AddVertsForDisc2D(std::vector<Vertex>& verts, Vec2 discCenter, float discRadius, Rgba8 color) 
{
	int numSides = (int)RangeMapClamped(discRadius, 1.f, 10.f, 36.f, 360.f);

    float delta = 360.0f / (float)numSides;
    for (int i = 0; i < numSides; ++i)
    {
        float angle0 = i * delta;
        float angle1 = (i + 1) * delta;
        Vec2 p0 = discCenter;
        Vec2 p1 = discCenter + Vec2::MakeFromPolarDegrees(angle0, discRadius);
        Vec2 p2 = discCenter + Vec2::MakeFromPolarDegrees(angle1, discRadius);
        verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
        verts.emplace_back(Vec3(p1.x, p1.y, 0.f), color);
        verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);
    }
}

void AddVertsForRing2D(std::vector<Vertex>& verts, Vec2 ringCenter, float ringRadius, float thickness, Rgba8 color) 
{
    int numSides = (int)RangeMapClamped(ringRadius, 1.f, 10.f, 36.f, 360.f);

    float innerRadius = ringRadius - thickness * 0.5f;
    float outerRadius = ringRadius + thickness * 0.5f;

    float delta = 360.0f / (float)numSides;
    for (int i = 0; i < numSides; ++i) 
    {
        float angle0 = i * delta;
        float angle1 = (i + 1) * delta;
        Vec2 inner0 = ringCenter + Vec2::MakeFromPolarDegrees(angle0, innerRadius);
        Vec2 inner1 = ringCenter + Vec2::MakeFromPolarDegrees(angle1, innerRadius);
        Vec2 outer0 = ringCenter + Vec2::MakeFromPolarDegrees(angle0, outerRadius);
        Vec2 outer1 = ringCenter + Vec2::MakeFromPolarDegrees(angle1, outerRadius);

        // First triangle
        verts.emplace_back(Vec3(inner0.x, inner0.y, 0.f), color);
        verts.emplace_back(Vec3(outer0.x, outer0.y, 0.f), color);
        verts.emplace_back(Vec3(outer1.x, outer1.y, 0.f), color);

        // Second triangle
        verts.emplace_back(Vec3(inner0.x, inner0.y, 0.f), color);
        verts.emplace_back(Vec3(outer1.x, outer1.y, 0.f), color);
        verts.emplace_back(Vec3(inner1.x, inner1.y, 0.f), color);
    }
}

void AddVertsForOBB2D(std::vector<Vertex>& verts, OBB2 const& orientedBox, Rgba8 color) 
{
    Vec2 corners[4];
    orientedBox.GetCornerPoints(corners);

    // Two triangles
    verts.emplace_back(Vec3(corners[0].x, corners[0].y, 0.f), color);
    verts.emplace_back(Vec3(corners[1].x, corners[1].y, 0.f), color);
    verts.emplace_back(Vec3(corners[2].x, corners[2].y, 0.f), color);

    verts.emplace_back(Vec3(corners[0].x, corners[0].y, 0.f), color);
    verts.emplace_back(Vec3(corners[2].x, corners[2].y, 0.f), color);
    verts.emplace_back(Vec3(corners[3].x, corners[3].y, 0.f), color);
}

void AddVertsForSector2D(std::vector<Vertex>& verts, Vec2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius, Rgba8 color)
{
    // Clamp aperture to [0, 360]
    float aperture = GetClamped(sectorApertureDegrees, 0.f, 360.f);
    if (aperture <= 0.f || sectorRadius <= 0.f) 
    {
        return;
    }

    // Determine number of sides based on aperture and radius (similar to disc)
    int sectorSides = (int)RangeMapClamped(sectorRadius, 1.f, 10.f, 36.f, 360.f);

    float startAngle = sectorForwardDegrees - aperture * 0.5f;
    float delta = aperture / (float)sectorSides;

    for (int i = 0; i < sectorSides; ++i) 
    {
        float angle0 = startAngle + i * delta;
        float angle1 = startAngle + (i + 1) * delta;
        Vec2 p0 = sectorOrigin;
        Vec2 p1 = sectorOrigin + Vec2::MakeFromPolarDegrees(angle0, sectorRadius);
        Vec2 p2 = sectorOrigin + Vec2::MakeFromPolarDegrees(angle1, sectorRadius);
        verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
        verts.emplace_back(Vec3(p1.x, p1.y, 0.f), color);
        verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);
    }
}

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Vec2 boneStart, Vec2 boneEnd, float radius, Rgba8 color)
{
    // Draw the body as a rectangle using AddVertsForLineSegment2D
    Vec2 dir = (boneEnd - boneStart).GetNormalized();
    Vec2 right = dir.GetRotatedBy90Degrees();
    Vec2 thicknessVec = right * (radius * 2.f);

	OBB2 capsuleBody = OBB2();
    capsuleBody.m_center = (boneStart + boneEnd) * 0.5f;
    capsuleBody.m_iBasisNormal = dir;
	capsuleBody.m_halfDimensions = Vec2((boneEnd - boneStart).GetLength() * 0.5f, radius);

	AddVertsForOBB2D(verts, capsuleBody, color);

    // Draw the semicircle at each end using AddVertsForSector2D
    float forwardDegrees = dir.GetOrientationDegrees();
    AddVertsForSector2D(verts, boneStart, forwardDegrees + 180.f, 180.f, radius, color);
    AddVertsForSector2D(verts, boneEnd, forwardDegrees, 180.f, radius, color);
}

void AddVertsForTriangle2D(std::vector<Vertex>& verts, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2, Rgba8 color) 
{
    verts.emplace_back(Vec3(ccw0.x, ccw0.y, 0.f), color);
    verts.emplace_back(Vec3(ccw1.x, ccw1.y, 0.f), color);
    verts.emplace_back(Vec3(ccw2.x, ccw2.y, 0.f), color);
}

void AddVertsForLineSegment2D(std::vector<Vertex>& verts, Vec2 start, Vec2 end, Vec2 thickness, Rgba8 color) 
{
    Vec2 dir = (end - start).GetNormalized();
    Vec2 right = dir.GetRotatedBy90Degrees();
    Vec2 halfThick = right * (thickness.x * 0.5f);

    Vec2 p0 = start + halfThick;
    Vec2 p1 = end + halfThick;
    Vec2 p2 = end - halfThick;
    Vec2 p3 = start - halfThick;

    verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
    verts.emplace_back(Vec3(p1.x, p1.y, 0.f), color);
    verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);

    verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
    verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);
    verts.emplace_back(Vec3(p3.x, p3.y, 0.f), color);
}

void AddVertsForInfiniteLine2D(std::vector<Vertex>& verts, Vec2 pointOnLine, Vec2 anotherPointOnLine, float thickness, Rgba8 color)
{
	Vec2 direction = anotherPointOnLine - pointOnLine;
    AddVertsForLineSegment2D(verts, pointOnLine - direction.GetNormalized() * 10000.f,
		pointOnLine + direction.GetNormalized() * 10000.f, Vec2(thickness, thickness), color);
}

void AddVertsForDisc2D(std::vector<Vertex>& verts, Disc2 const& disc, Rgba8 color) 
{
    AddVertsForDisc2D(verts, disc.m_center, disc.m_radius, color);
}

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Capsule2 const& capsule, Rgba8 color) 
{
    AddVertsForCapsule2D(verts, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius, color);
}

void AddVertsForTriangle2D(std::vector<Vertex>& verts, Triangle2 const& triangle, Rgba8 color) 
{
    AddVertsForTriangle2D(verts, triangle.m_pointsCounterClockwise[0], triangle.m_pointsCounterClockwise[1], triangle.m_pointsCounterClockwise[2], color);
}

void AddVertsForLineSegment2D(std::vector<Vertex>& verts, LineSegment2 const& lineSegment, float thickness, Rgba8 color) 
{
	AddVertsForLineSegment2D(verts, lineSegment.m_start, lineSegment.m_end, Vec2(thickness, thickness), color);
}

void AddVertsForInfiniteLine2D(std::vector<Vertex>& verts, LineSegment2 const& infiniteLine, float thickness, Rgba8 color)
{
	AddVertsForInfiniteLine2D(verts, infiniteLine.m_start, infiniteLine.m_end, thickness, color);
}
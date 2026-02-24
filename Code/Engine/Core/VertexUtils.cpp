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
    AddVertsForAABB2D(verts, alignedBox, color, Vec2::ZERO, Vec2::ONE);
}

void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Rgba8 color, Vec2 uvAtMins, Vec2 uvAtMaxs)
{
    Vec2 mins = alignedBox.m_mins;
    Vec2 maxs = alignedBox.m_maxs;

    verts.emplace_back(Vec3(mins.x, mins.y, 0.f), color, Vec2(uvAtMins.x, uvAtMins.y));
    verts.emplace_back(Vec3(maxs.x, mins.y, 0.f), color, Vec2(uvAtMaxs.x, uvAtMins.y));
    verts.emplace_back(Vec3(maxs.x, maxs.y, 0.f), color, Vec2(uvAtMaxs.x, uvAtMaxs.y));

    verts.emplace_back(Vec3(mins.x, mins.y, 0.f), color, Vec2(uvAtMins.x, uvAtMins.y));
    verts.emplace_back(Vec3(maxs.x, maxs.y, 0.f), color, Vec2(uvAtMaxs.x, uvAtMaxs.y));
    verts.emplace_back(Vec3(mins.x, maxs.y, 0.f), color, Vec2(uvAtMins.x, uvAtMaxs.y));
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

void AddVertsForArrow2D(std::vector<Vertex>& verts, Vec2 tailPos, Vec2 tipPos, float arrowSize, float lineThickness, Rgba8 color)
{
    Vec2 dir = (tipPos - tailPos).GetNormalized();
    float length = (tipPos - tailPos).GetLength();

    float headLength = (arrowSize > 0.f) ? std::min(arrowSize, length * 0.5f) : length * 0.2f;
    float shaftLength = length - headLength;

    Vec2 headBase = tipPos - dir * headLength;

    Vec2 right = dir.GetRotatedBy90Degrees();
    Vec2 halfShaft = right * (lineThickness * 0.5f);
    float headWidth = headLength;
    Vec2 halfHead = right * (headWidth * 0.5f);

    if (shaftLength > 0.0f)
    {
        Vec2 shaftStartL = tailPos + halfShaft;
        Vec2 shaftStartR = tailPos - halfShaft;
        Vec2 shaftEndL = headBase + halfShaft;
        Vec2 shaftEndR = headBase - halfShaft;

        verts.emplace_back(Vec3(shaftStartL.x, shaftStartL.y, 0.f), color);
        verts.emplace_back(Vec3(shaftEndL.x, shaftEndL.y, 0.f), color);
        verts.emplace_back(Vec3(shaftEndR.x, shaftEndR.y, 0.f), color);

        verts.emplace_back(Vec3(shaftStartL.x, shaftStartL.y, 0.f), color);
        verts.emplace_back(Vec3(shaftEndR.x, shaftEndR.y, 0.f), color);
        verts.emplace_back(Vec3(shaftStartR.x, shaftStartR.y, 0.f), color);
    }

    Vec2 headLeft = headBase + halfHead;
    Vec2 headRight = headBase - halfHead;

    verts.emplace_back(Vec3(tipPos.x, tipPos.y, 0.f), color);
    verts.emplace_back(Vec3(headLeft.x, headLeft.y, 0.f), color);
    verts.emplace_back(Vec3(headRight.x, headRight.y, 0.f), color);
}

void AddVertsForQuad3D(std::vector<Vertex>& verts, const Vec3& bottomLeft, const Vec3& bottomRight, const Vec3& topRight, const Vec3& topLeft, const Rgba8& color /*= Rgba8::WHITE*/, const AABB2& UVs /*= AABB2::UNIT */)
{
    Vec2 uvMins = UVs.m_mins;
    Vec2 uvMaxs = UVs.m_maxs;

    verts.emplace_back(bottomLeft, color, Vec2(uvMins.x, uvMins.y));
    verts.emplace_back(bottomRight, color, Vec2(uvMaxs.x, uvMins.y));
    verts.emplace_back(topRight, color, Vec2(uvMaxs.x, uvMaxs.y));

    verts.emplace_back(bottomLeft, color, Vec2(uvMins.x, uvMins.y));
    verts.emplace_back(topRight, color, Vec2(uvMaxs.x, uvMaxs.y));
    verts.emplace_back(topLeft, color, Vec2(uvMins.x, uvMaxs.y));
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
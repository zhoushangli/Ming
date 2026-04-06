#include "Engine/Math/MathUtils.hpp"

#include <math.h>

float Abs(float value)
{
    return (value < 0.f) ? -value : value;
}

float Sign(float value)
{
    if (value > 0.f)
    {
        return 1.f;
    }
    else if (value < 0.f)
    {
        return -1.f;
    }
    else
    {
        return 0.f;
    }
}

float Min(float a, float b)
{
    return (a < b) ? a : b;
}

int Min(int a, int b)
{
    return (a < b) ? a : b;
}

float Max(float a, float b)
{
    return (a > b) ? a : b;
}

int Max(int a, int b)
{
    return (a > b) ? a : b;
}

float ConvertDegreesToRadians(float degrees)
{
    return degrees * DegreesToRadiansMultiplier;
}

float ConvertRadiansToDegrees(float radians)
{
    return radians * RadiansToDegreesMultiplier;
}

float CosDegrees(float degrees)
{
    return cosf(ConvertDegreesToRadians(degrees));
}

float SinDegrees(float degrees)
{
    return sinf(ConvertDegreesToRadians(degrees));
}

float Atan2Degrees(float y, float x)
{
    return ConvertRadiansToDegrees(atan2f(y, x));
}

float GetDistance2D(Vec2 const &a, Vec2 const &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float GetDistanceSquared2D(Vec2 const &a, Vec2 const &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

float GetDistance3D(Vec3 const &a, Vec3 const &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

float GetDistanceXY3D(Vec3 const &a, Vec3 const &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float GetDistanceSquared3D(Vec3 const &a, Vec3 const &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

float GetDistanceXYSquared3D(Vec3 const &a, Vec3 const &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool DoDiscsOverlap2D(Vec2 const &centerA, float radiusA, Vec2 const &centerB, float radiusB)
{
    float distSquared = GetDistanceSquared2D(centerA, centerB);
    float radiiSum = radiusA + radiusB;
    return distSquared <= (radiiSum * radiiSum);
}

bool DoAABB3sOverlap3D(Vec3 const &firstMins, Vec3 const &firstMaxs, Vec3 const &secondMins, Vec3 const &secondMaxs)
{
    bool overlapsX = (firstMins.x < secondMaxs.x) && (secondMins.x < firstMaxs.x);
    bool overlapsY = (firstMins.y < secondMaxs.y) && (secondMins.y < firstMaxs.y);
    bool overlapsZ = (firstMins.z < secondMaxs.z) && (secondMins.z < firstMaxs.z);
    return overlapsX && overlapsY && overlapsZ;
}

bool DoAABB3sOverlap3D(AABB3 const &first, AABB3 const &second)
{
    return DoAABB3sOverlap3D(first.m_mins, first.m_maxs, second.m_mins, second.m_maxs);
}

bool DoSpheresOverlap3D(Vec3 const &centerA, float radiusA, Vec3 const &centerB, float radiusB)
{
    float distSquared = GetDistanceSquared3D(centerA, centerB);
    float radiiSum = radiusA + radiusB;
    return distSquared < (radiiSum * radiiSum);
}

bool DoSpheresOverlap3D(Sphere3 const &a, Sphere3 const &b)
{
    return DoSpheresOverlap3D(a.m_center, a.m_radius, b.m_center, b.m_radius);
}

bool DoCylinderZsOverlap3D(Vec2 const &cylinder1CenterXY, float cylinder1Radius, FloatRange const &cylinder1MinMaxZ,
                           Vec2 const &cylinder2CenterXY, float cylinder2Radius, FloatRange const &cylinder2MinMaxZ)
{
    float distSquaredXY = GetDistanceSquared2D(cylinder1CenterXY, cylinder2CenterXY);
    float radiiSum = cylinder1Radius + cylinder2Radius;
    bool overlapInXY = distSquaredXY < (radiiSum * radiiSum);

    bool overlapInZ = cylinder1MinMaxZ.IsOverlap(cylinder2MinMaxZ);
    return overlapInXY && overlapInZ;
}

bool DoCylinderZsOverlap3D(Vec3 const &centerA, float radiusA, float heightA, Vec3 const &centerB, float radiusB, float heightB)
{
    FloatRange rangeA(centerA.z - (heightA * 0.5f), centerA.z + (heightA * 0.5f));
    FloatRange rangeB(centerB.z - (heightB * 0.5f), centerB.z + (heightB * 0.5f));
    return DoCylinderZsOverlap3D(Vec2(centerA.x, centerA.y), radiusA, rangeA, Vec2(centerB.x, centerB.y), radiusB, rangeB);
}

bool DoCylinderZsOverlap3D(CylinderZ3 const &a, CylinderZ3 const &b)
{
    FloatRange rangeA = a.GetMinMaxZ();
    FloatRange rangeB = b.GetMinMaxZ();
    return DoCylinderZsOverlap3D(Vec2(a.m_start.x, a.m_start.y), a.m_radius, rangeA, Vec2(b.m_start.x, b.m_start.y), b.m_radius, rangeB);
}

bool DoSphereAndAABBOverlap3D(Vec3 const &sphereCenter, float sphereRadius, Vec3 const &boxMins, Vec3 const &boxMaxs)
{
    Vec3 nearestPoint = GetNearestPointOnAABB3D(sphereCenter, boxMins, boxMaxs);
    float distanceSquared = GetDistanceSquared3D(sphereCenter, nearestPoint);
    return distanceSquared < (sphereRadius * sphereRadius);
}

bool DoSphereAndAABBOverlap3D(Vec3 const &sphereCenter, float sphereRadius, AABB3 const &box)
{
    return DoSphereAndAABBOverlap3D(sphereCenter, sphereRadius, box.m_mins, box.m_maxs);
}

bool DoSphereAndAABBOverlap3D(Sphere3 const &sphere, AABB3 const &box)
{
    return DoSphereAndAABBOverlap3D(sphere.m_center, sphere.m_radius, box);
}

bool DoCylinderZAndAABBOverlap3D(Vec2 const &cylinderCenterXY, float cylinderRadius, FloatRange const &cylinderMinMaxZ, Vec3 const &boxMins, Vec3 const &boxMaxs)
{
    float nearestX = GetClamped(cylinderCenterXY.x, boxMins.x, boxMaxs.x);
    float nearestY = GetClamped(cylinderCenterXY.y, boxMins.y, boxMaxs.y);
    float dx = cylinderCenterXY.x - nearestX;
    float dy = cylinderCenterXY.y - nearestY;
    bool overlapInXY = (dx * dx + dy * dy) < (cylinderRadius * cylinderRadius);

    FloatRange boxRangeZ(boxMins.z, boxMaxs.z);
    bool overlapInZ = cylinderMinMaxZ.IsOverlap(boxRangeZ);
    return overlapInXY && overlapInZ;
}

bool DoCylinderZAndAABBOverlap3D(Vec2 const &cylinderCenterXY, float cylinderRadius, FloatRange const &cylinderMinMaxZ, AABB3 const &box)
{
    return DoCylinderZAndAABBOverlap3D(cylinderCenterXY, cylinderRadius, cylinderMinMaxZ, box.m_mins, box.m_maxs);
}

bool DoCylinderZAndAABBOverlap3D(CylinderZ3 const &cylinder, AABB3 const &box)
{
    FloatRange cylinderRangeZ = cylinder.GetMinMaxZ();
    return DoCylinderZAndAABBOverlap3D(Vec2(cylinder.m_start.x, cylinder.m_start.y), cylinder.m_radius, cylinderRangeZ, box);
}

bool DoCylinderZAndSphereOverlap3D(Vec2 const &cylinderCenterXY, float cylinderRadius, FloatRange const &cylinderMinMaxZ, Vec3 const &sphereCenter, float sphereRadius)
{
    float cylinderMinZ = cylinderMinMaxZ.m_min;
    float cylinderMaxZ = cylinderMinMaxZ.m_max;
    Vec3 cylinderStart(cylinderCenterXY.x, cylinderCenterXY.y, cylinderMinZ);
    float cylinderHeight = cylinderMaxZ - cylinderMinZ;

    Vec3 nearestPoint = GetNearestPointOnZCylinder3D(sphereCenter, cylinderStart, cylinderHeight, cylinderRadius);
    float distanceSquared = GetDistanceSquared3D(sphereCenter, nearestPoint);
    return distanceSquared < (sphereRadius * sphereRadius);
}

bool DoCylinderZAndSphereOverlap3D(CylinderZ3 const &cylinder, Sphere3 const &sphere)
{
    FloatRange cylinderRangeZ = cylinder.GetMinMaxZ();
    return DoCylinderZAndSphereOverlap3D(Vec2(cylinder.m_start.x, cylinder.m_start.y), cylinder.m_radius, cylinderRangeZ, sphere.m_center, sphere.m_radius);
}

void TransformPosition2D(Vec2 &pos, float scale, float rotationDegrees, Vec2 const &translation)
{
    pos *= scale;
    pos.RotateDegrees(rotationDegrees);
    pos += translation;
}

void TransformPosition2D(Vec2 &pos, Vec2 const &iBasis, Vec2 const &jBasis, Vec2 const &translation)
{
    float x = pos.x;
    float y = pos.y;
    pos = iBasis * x + jBasis * y + translation;
}

void TransformPositionXY3D(Vec3 &pos, float scaleXY, float zRotationDegrees, Vec2 const &translationXY)
{
    pos.x *= scaleXY;
    pos.y *= scaleXY;
    pos = pos.GetRotatedAboutZDegrees(zRotationDegrees);
    pos.x += translationXY.x;
    pos.y += translationXY.y;
}

void TransformPositionXY3D(Vec3 &pos, Vec2 const &iBasisXY, Vec2 const &jBasisXY, Vec2 const &translationXY)
{
    float x = pos.x;
    float y = pos.y;
    Vec2 pos2D = iBasisXY * x + jBasisXY * y + translationXY;
    pos.x = pos2D.x;
    pos.y = pos2D.y;
}

float Interpolate(float start, float end, float fraction)
{
    return start * (1.0f - fraction) + end * fraction;
}

float InterpolateClamped(float start, float end, float fraction)
{
    float f = GetClampedZeroToOne(fraction);
    return Interpolate(start, end, f);
}

float GetFractionWithinRange(float value, float start, float end)
{
    return (value - start) / (end - start);
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
    return Interpolate(outStart, outEnd, fraction);
}

float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
    fraction = GetClampedZeroToOne(fraction);
    return Interpolate(outStart, outEnd, fraction);
}

float GetClamped(float value, float minValue, float maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }

    if (value > maxValue)
    {
        return maxValue;
    }

    return value;
}

int GetClamped(int value, int minValue, int maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }

    if (value > maxValue)
    {
        return maxValue;
    }

    return value;
}

float GetClampedZeroToOne(float value)
{
    if (value < 0.f)
    {
        return 0.f;
    }

    if (value > 1.f)
    {
        return 1.f;
    }

    return value;
}

int RoundDownToInt(float value)
{
    if (value >= 0.f)
    {
        return static_cast<int>(value);
    }
    else
    {
        int intValue = static_cast<int>(value);
        if (static_cast<float>(intValue) == value)
        {
            return intValue;
        }
        else
        {
            return intValue - 1;
        }
    }
}

/// <summary>
/// Returns the shortest angular displacement from startDegrees to endDegrees in the range [-180, 180].
/// </summary>
/// <param name="startDegrees"></param>
/// <param name="endDegrees"></param>
/// <returns></returns>
float GetShortestAngularDispDegrees(float startDegrees, float endDegrees)
{
    float delta = endDegrees - startDegrees;

    while (delta > 180.f)
    {
        delta -= 360.f;
    }

    while (delta < -180.f)
    {
        delta += 360.f;
    }

    return delta;
}

float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees)
{
    float delta = GetShortestAngularDispDegrees(currentDegrees, goalDegrees);
    if (delta > maxDeltaDegrees)
    {
        return currentDegrees + maxDeltaDegrees;
    }
    if (delta < -maxDeltaDegrees)
    {
        return currentDegrees - maxDeltaDegrees;
    }
    else
    {
        return goalDegrees;
    }
}

float DotProduct2D(Vec2 const &a, Vec2 const &b)
{
    return a.x * b.x + a.y * b.y;
}

float DotProduct3D(Vec3 const &a, Vec3 const &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float DotProduct4D(Vec4 const &a, Vec4 const &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float CrossProduct2D(Vec2 const &a, Vec2 const &b)
{
    return a.x * b.y - a.y * b.x;
}

Vec3 CrossProduct3D(Vec3 const &a, Vec3 const &b)
{
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

Matrix4x4 GetBillboardTransform(BillboardType billboardType, Matrix4x4 const &targetTransform, const Vec3 &billboardPosition, const Vec2 &billboardScale /*= Vec2(1.0f, 1.0f)*/)
{
    Vec3 const billboardScale3D = Vec3(billboardScale.x, billboardScale.y, 1.0f);

    Matrix4x4 billboardTransform = Matrix4x4::MakeTranslation3D(billboardPosition);
    billboardTransform.AppendScaleNonUniform3D(billboardScale3D);

    switch (billboardType)
    {
    case BillboardType::NONE:
    {
        return billboardTransform;
    }

    case BillboardType::WORLD_UP_FACING:
    {
        Vec3 worldUp = Vec3::UP;
        Vec3 toTarget = targetTransform.GetTranslation3D() - billboardPosition;
        Vec3 toTargetOnXY = toTarget - GetProjectedVector3D(toTarget, worldUp);

        if (toTargetOnXY.GetLengthSquared() <= 1e-5f)
        {
            toTarget = targetTransform.GetIBasis3D();
            toTargetOnXY = toTarget - GetProjectedVector3D(toTarget, worldUp);

            if (toTargetOnXY.GetLengthSquared() <= 1e-5f)
            {
                toTarget = Vec3::FORWARD;
                toTargetOnXY = toTarget - GetProjectedVector3D(toTarget, worldUp);
            }
        }

        Vec3 iBasis = toTargetOnXY.GetNormalized();
        Vec3 jBasis = CrossProduct3D(worldUp, iBasis);
        jBasis.Normalize();

        billboardTransform = Matrix4x4(iBasis, jBasis, worldUp, billboardPosition);
        billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
        return billboardTransform;
    }

    case BillboardType::WORLD_UP_OPPOSING:
    {
        Vec3 worldUp = Vec3::UP;
        Vec3 invTarget = -targetTransform.GetIBasis3D();
        Vec3 fromTargetOnXY = invTarget - GetProjectedVector3D(invTarget, worldUp);

        if (fromTargetOnXY.GetLengthSquared() <= 1e-5f)
        {
            invTarget = targetTransform.GetIBasis3D();
            fromTargetOnXY = invTarget - GetProjectedVector3D(invTarget, worldUp);

            if (fromTargetOnXY.GetLengthSquared() <= 1e-5f)
            {
                invTarget = Vec3::FORWARD;
                fromTargetOnXY = invTarget - GetProjectedVector3D(invTarget, worldUp);
            }
        }

        Vec3 iBasis = fromTargetOnXY.GetNormalized();
        Vec3 jBasis = CrossProduct3D(worldUp, iBasis);
        jBasis.Normalize();

        billboardTransform = Matrix4x4(iBasis, jBasis, worldUp, billboardPosition);
        billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
        return billboardTransform;
    }

    case BillboardType::FULL_FACING:
    {
        Vec3 toTarget = targetTransform.GetTranslation3D() - billboardPosition;
        Vec3 iBasis = toTarget.GetNormalized();
        Vec3 referenceUp = targetTransform.GetKBasis3D();
        if (referenceUp.GetLengthSquared() <= 1e-5f)
        {
            referenceUp = Vec3::UP;
        }

        Vec3 jBasis = CrossProduct3D(referenceUp, iBasis);
        if (jBasis.GetLengthSquared() <= 1e-5f)
        {
            referenceUp = targetTransform.GetJBasis3D();
            jBasis = CrossProduct3D(referenceUp, iBasis);

            if (jBasis.GetLengthSquared() <= 1e-5f)
            {
                referenceUp = Vec3::UP;
                jBasis = CrossProduct3D(referenceUp, iBasis);
            }
        }

        jBasis.Normalize();
        Vec3 kBasis = CrossProduct3D(iBasis, jBasis);
        kBasis.Normalize();

        billboardTransform = Matrix4x4(iBasis, jBasis, kBasis, billboardPosition);
        billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
        return billboardTransform;
    }

    case BillboardType::FULL_OPPOSING:
    {
        Vec3 invTarget = -targetTransform.GetIBasis3D();
        Vec3 iBasis = invTarget.GetNormalized();
        Vec3 referenceUp = targetTransform.GetKBasis3D();
        if (referenceUp.GetLengthSquared() <= 1e-5f)
        {
            referenceUp = Vec3::UP;
        }

        Vec3 jBasis = CrossProduct3D(referenceUp, iBasis);
        if (jBasis.GetLengthSquared() <= 1e-5f)
        {
            referenceUp = targetTransform.GetJBasis3D();
            jBasis = CrossProduct3D(referenceUp, iBasis);

            if (jBasis.GetLengthSquared() <= 1e-5f)
            {
                referenceUp = Vec3::UP;
                jBasis = CrossProduct3D(referenceUp, iBasis);
            }
        }

        jBasis.Normalize();
        Vec3 kBasis = CrossProduct3D(iBasis, jBasis);
        kBasis.Normalize();

        billboardTransform = Matrix4x4(iBasis, jBasis, kBasis, billboardPosition);
        billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
        return billboardTransform;
    }

    default:
        return billboardTransform;
    }
}

bool PushDiscOutOfFixedPoint2D(Vec2 &discCenter, float discRadius, Vec2 const &fixedPoint)
{
    Vec2 toCenter = discCenter - fixedPoint;
    float dist = toCenter.GetLength();

    if (dist >= discRadius || dist == 0.f)
    {
        return false;
    }

    Vec2 pushDir = toCenter.GetNormalized();
    discCenter = fixedPoint + pushDir * discRadius;
    return true;
}

bool PushDiscOutOfFixedDisc2D(Vec2 &discCenter, float discRadius, Vec2 const &fixedDiscCenter, float fixedDiscRadius)
{
    Vec2 between = discCenter - fixedDiscCenter;
    float dist = between.GetLength();
    float minDist = discRadius + fixedDiscRadius;

    if (dist >= minDist || dist == 0.f)
    {
        return false;
    }

    Vec2 pushDir = between.GetNormalized();
    discCenter = fixedDiscCenter + pushDir * minDist;
    return true;
}

bool PushDiscOutOfFixedDisc2D(Disc2 &discToPush, Disc2 const &fixedDisc)
{
    return PushDiscOutOfFixedDisc2D(discToPush.m_center, discToPush.m_radius, fixedDisc.m_center, fixedDisc.m_radius);
}

bool PushDiscsOutOfEachOther2D(Vec2 &discCenterA, float discRadiusA, Vec2 &discCenterB, float discRadiusB)
{
    Vec2 between = discCenterA - discCenterB;
    float dist = between.GetLength();
    float minDist = discRadiusA + discRadiusB;

    if (dist >= minDist || dist == 0.f)
    {
        return false;
    }

    Vec2 pushDir = between.GetNormalized();
    float overlap = minDist - dist;
    discCenterA += pushDir * (overlap * 0.5f);
    discCenterB -= pushDir * (overlap * 0.5f);
    return true;
}

bool PushDiscsOutOfEachOther2D(Disc2 &discA, Disc2 &discB)
{
    return PushDiscsOutOfEachOther2D(discA.m_center, discA.m_radius, discB.m_center, discB.m_radius);
}

bool PushDiscOutOfFixedAABB2D(Vec2 &discCenter, float discRadius, AABB2 const &box)
{
    Vec2 nearest = box.GetNearestPoint(discCenter);
    Vec2 toCenter = discCenter - nearest;
    float dist = toCenter.GetLength();

    if (dist >= discRadius || dist == 0.f)
    {
        return false;
    }

    Vec2 pushDir = toCenter.GetNormalized();
    discCenter = nearest + pushDir * discRadius;
    return true;
}

float GetProjectedLength2D(Vec2 const &vector, Vec2 const &basis)
{
    Vec2 n = basis.GetNormalized();
    return DotProduct2D(vector, n);
}

Vec2 GetProjectedVector2D(Vec2 const &vector, Vec2 const &basis)
{
    Vec2 n = basis.GetNormalized();
    return n * DotProduct2D(vector, n);
}

Vec3 GetProjectedVector3D(Vec3 const &vector, Vec3 const &basis)
{
    Vec3 n = basis.GetNormalized();
    return n * DotProduct3D(vector, n);
}

float GetAngleDegreesBetweenVectors2D(Vec2 const &a, Vec2 const &b)
{
    float aLen = a.GetLength();
    float bLen = b.GetLength();
    if (aLen == 0.f || bLen == 0.f)
        return 0.f;
    float dot = DotProduct2D(a, b) / (aLen * bLen);
    dot = GetClamped(dot, -1.f, 1.f);
    return ConvertRadiansToDegrees(acosf(dot));
}

int GetTaxicabDistance2D(IntVec2 const &a, IntVec2 const &b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

int GetTaxicabDistance2D(Vec2 const &a, Vec2 const &b)
{
    IntVec2 intA(RoundDownToInt(a.x), RoundDownToInt(a.y));
    IntVec2 intB(RoundDownToInt(b.x), RoundDownToInt(b.y));
    return GetTaxicabDistance2D(intA, intB);
}

// --- Is Point Inside ---
bool IsPointInsideDisc2D(Vec2 point, Vec2 discCenter, float discRadius)
{
    Vec2 toPoint = point - discCenter;
    float distSquared = toPoint.GetLengthSquared();
    return distSquared < (discRadius * discRadius);
}

bool IsPointInsideDisc2D(Vec2 point, Disc2 const &disc)
{
    return IsPointInsideDisc2D(point, disc.m_center, disc.m_radius);
}

bool IsPointInsideAABB2D(Vec2 point, AABB2 const &alignedBox)
{
    return (point.x > alignedBox.m_mins.x && point.x < alignedBox.m_maxs.x &&
            point.y > alignedBox.m_mins.y && point.y < alignedBox.m_maxs.y);
}

bool IsPointInsideOBB2D(Vec2 point, OBB2 const &orientedBox)
{
    Vec2 localPos = orientedBox.GetLocalPosForWorldPos(point);
    return IsPointInsideAABB2D(localPos, AABB2(-orientedBox.m_halfDimensions, orientedBox.m_halfDimensions));
}

bool IsPointInsideCapsule2D(Vec2 point, Vec2 boneStart, Vec2 boneEnd, float radius)
{
    Vec2 nearestPoint = GetNearestPointOnLineSegment2D(point, boneStart, boneEnd);
    return IsPointInsideDisc2D(point, nearestPoint, radius);
}

bool IsPointInsideCapsule2D(Vec2 point, Capsule2 const &capsule)
{
    return IsPointInsideCapsule2D(point, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius);
}

bool IsPointInsideTriangle2D(Vec2 point, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2)
{
    Vec2 edge0 = ccw1 - ccw0;
    Vec2 edge1 = ccw2 - ccw1;
    Vec2 edge2 = ccw0 - ccw2;

    Vec2 edge0Rotate90 = edge0.GetRotatedBy90Degrees();
    Vec2 edge1Rotate90 = edge1.GetRotatedBy90Degrees();
    Vec2 edge2Rotate90 = edge2.GetRotatedBy90Degrees();

    Vec2 toPoint0 = point - ccw0;
    Vec2 toPoint1 = point - ccw1;
    Vec2 toPoint2 = point - ccw2;

    if (DotProduct2D(edge0Rotate90, toPoint0) < 0.f)
        return false;
    if (DotProduct2D(edge1Rotate90, toPoint1) < 0.f)
        return false;
    if (DotProduct2D(edge2Rotate90, toPoint2) < 0.f)
        return false;

    return true;
}

bool IsPointInsideTriangle2D(Vec2 point, Triangle2 const &triangle)
{
    return IsPointInsideTriangle2D(point, triangle.m_pointsCounterClockwise[0], triangle.m_pointsCounterClockwise[1], triangle.m_pointsCounterClockwise[2]);
}

bool IsPointInsideOrientedSector2D(Vec2 point, Vec2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius)
{
    if (!IsPointInsideDisc2D(point, sectorOrigin, sectorRadius))
    {
        return false;
    }

    Vec2 toPoint = point - sectorOrigin;
    Vec2 forward = Vec2::MakeFromPolarDegrees(sectorForwardDegrees, 1.f);
    float angle = GetAngleDegreesBetweenVectors2D(toPoint, forward);
    return angle < (sectorApertureDegrees * 0.5f);
}

bool IsPointInsideDirectedSector2D(Vec2 point, Vec2 sectorOrigin, Vec2 sectorForwardNormal, float sectorApertureDegrees, float sectorRadius)
{
    if (!IsPointInsideDisc2D(point, sectorOrigin, sectorRadius))
    {
        return false;
    }

    Vec2 toPoint = point - sectorOrigin;
    Vec2 dirToPoint = toPoint.GetNormalized();
    Vec2 fwd = sectorForwardNormal.GetNormalized();
    float cosAngle = DotProduct2D(dirToPoint, fwd);
    float cosLimit = CosDegrees(sectorApertureDegrees * 0.5f);
    return cosAngle > cosLimit;
}

// --- Get Nearest Point On ---
Vec2 GetNearestPointOnDisc2D(Vec2 point, Vec2 discCenter, float discRadius)
{
    Vec2 toPoint = point - discCenter;
    float dist = toPoint.GetLength();

    if (dist <= discRadius || dist == 0.f)
    {
        return point;
    }

    return discCenter + toPoint * (discRadius / dist);
}

Vec2 GetNearestPointOnDisc2D(Vec2 referencePos, Disc2 const &disc)
{
    return GetNearestPointOnDisc2D(referencePos, disc.m_center, disc.m_radius);
}

Vec2 GetNearestPointOnAABB2D(Vec2 referencePos, AABB2 const &alignedBox)
{
    return Vec2(
        GetClamped(referencePos.x, alignedBox.m_mins.x, alignedBox.m_maxs.x),
        GetClamped(referencePos.y, alignedBox.m_mins.y, alignedBox.m_maxs.y));
}

Vec2 GetNearestPointOnOBB2D(Vec2 referencePos, OBB2 const &orientedBox)
{
    Vec2 localPos = orientedBox.GetLocalPosForWorldPos(referencePos);
    Vec2 clampedLocalPos = Vec2(
        GetClamped(localPos.x, -orientedBox.m_halfDimensions.x, orientedBox.m_halfDimensions.x),
        GetClamped(localPos.y, -orientedBox.m_halfDimensions.y, orientedBox.m_halfDimensions.y));
    return orientedBox.GetWorldPosForLocalPos(clampedLocalPos);
}

Vec2 GetNearestPointOnInfiniteLine2D(Vec2 referencePos, Vec2 pointOnLine, Vec2 anotherPointOnLine)
{
    Vec2 lineDir = (anotherPointOnLine - pointOnLine).GetNormalized();

    if (lineDir.GetLengthSquared() == 0.f)
    {
        return pointOnLine;
    }

    Vec2 toReference = referencePos - pointOnLine;
    float projectedLength = DotProduct2D(toReference, lineDir);
    return pointOnLine + lineDir * projectedLength;
}

Vec2 GetNearestPointOnInfiniteLine2D(Vec2 referencePos, LineSegment2 const &lineSegmentOnInfiniteLine)
{
    return GetNearestPointOnInfiniteLine2D(referencePos, lineSegmentOnInfiniteLine.m_start, lineSegmentOnInfiniteLine.m_end);
}

Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, Vec2 start, Vec2 end)
{
    Vec2 startToEnd = end - start;
    Vec2 endToStart = start - end;

    Vec2 startToRef = referencePos - start;
    Vec2 endToRef = referencePos - end;

    if (DotProduct2D(startToRef, startToEnd) <= 0.f)
        return start;
    if (DotProduct2D(endToRef, endToStart) <= 0.f)
        return end;

    return GetNearestPointOnInfiniteLine2D(referencePos, start, end);
}

Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, LineSegment2 const &lineSegment)
{
    return GetNearestPointOnLineSegment2D(referencePos, lineSegment.m_start, lineSegment.m_end);
}

Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Vec2 boneStart, Vec2 boneEnd, float radius)
{
    Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(referencePos, boneStart, boneEnd);
    return GetNearestPointOnDisc2D(referencePos, nearestPointOnBone, radius);
}

Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Capsule2 const &capsule)
{
    return GetNearestPointOnCapsule2D(referencePos, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius);
}

Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2)
{
    if (IsPointInsideTriangle2D(referencePos, ccw0, ccw1, ccw2))
    {
        return referencePos;
    }

    Vec2 nearest0 = GetNearestPointOnLineSegment2D(referencePos, ccw0, ccw1);
    Vec2 nearest1 = GetNearestPointOnLineSegment2D(referencePos, ccw1, ccw2);
    Vec2 nearest2 = GetNearestPointOnLineSegment2D(referencePos, ccw2, ccw0);

    float distSquared0 = GetDistanceSquared2D(referencePos, nearest0);
    float distSquared1 = GetDistanceSquared2D(referencePos, nearest1);
    float distSquared2 = GetDistanceSquared2D(referencePos, nearest2);

    if (distSquared0 <= distSquared1 && distSquared0 <= distSquared2)
    {
        return nearest0;
    }
    else if (distSquared1 <= distSquared0 && distSquared1 <= distSquared2)
    {
        return nearest1;
    }
    else
    {
        return nearest2;
    }
}

Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Triangle2 const &triangle)
{
    return GetNearestPointOnTriangle2D(referencePos, triangle.m_pointsCounterClockwise[0], triangle.m_pointsCounterClockwise[1], triangle.m_pointsCounterClockwise[2]);
}

Vec3 GetNearestPointOnAABB3D(Vec3 referencePos, AABB3 const &alignedBox)
{
    return Vec3(
        GetClamped(referencePos.x, alignedBox.m_mins.x, alignedBox.m_maxs.x),
        GetClamped(referencePos.y, alignedBox.m_mins.y, alignedBox.m_maxs.y),
        GetClamped(referencePos.z, alignedBox.m_mins.z, alignedBox.m_maxs.z));
}

Vec3 GetNearestPointOnAABB3D(Vec3 referencePos, Vec3 const &boxMins, Vec3 const &boxMaxs)
{
    return Vec3(
        GetClamped(referencePos.x, boxMins.x, boxMaxs.x),
        GetClamped(referencePos.y, boxMins.y, boxMaxs.y),
        GetClamped(referencePos.z, boxMins.z, boxMaxs.z));
}

Vec3 GetNearestPointOnZCylinder3D(Vec3 referencePos, CylinderZ3 const &cylinder)
{
    return GetNearestPointOnZCylinder3D(referencePos, cylinder.m_start, cylinder.m_height, cylinder.m_radius);
}

Vec3 GetNearestPointOnZCylinder3D(Vec3 referencePos, Vec3 const &cylinderStart, float cylinderHeight, float cylinderRadius)
{
    float const localZ = referencePos.z - cylinderStart.z;
    float const clampedLocalZ = GetClamped(localZ, 0.f, cylinderHeight);

    Vec3 radialDisplacement(referencePos.x - cylinderStart.x, referencePos.y - cylinderStart.y, 0.f);
    float const radialDistanceSquared = radialDisplacement.GetLengthXYSquared();
    float const radiusSquared = cylinderRadius * cylinderRadius;

    if (localZ == clampedLocalZ && radialDistanceSquared <= radiusSquared)
    {
        return referencePos;
    }

    Vec3 nearest(cylinderStart.x, cylinderStart.y, cylinderStart.z + clampedLocalZ);
    if (radialDistanceSquared <= radiusSquared)
    {
        nearest.x = referencePos.x;
        nearest.y = referencePos.y;
        return nearest;
    }

    float const radialDistance = sqrtf(radialDistanceSquared);
    if (radialDistance > 0.f)
    {
        float const radialScale = cylinderRadius / radialDistance;
        nearest.x = cylinderStart.x + radialDisplacement.x * radialScale;
        nearest.y = cylinderStart.y + radialDisplacement.y * radialScale;
    }

    return nearest;
}

Vec3 GetNearestPointOnSphere3D(Vec3 referencePos, Sphere3 const &sphere)
{
    return GetNearestPointOnSphere3D(referencePos, sphere.m_center, sphere.m_radius);
}

Vec3 GetNearestPointOnSphere3D(Vec3 referencePos, Vec3 const &sphereCenter, float sphereRadius)
{
    Vec3 displacement = referencePos - sphereCenter;
    float const distanceSquared = displacement.GetLengthSquared();
    float const radiusSquared = sphereRadius * sphereRadius;
    if (distanceSquared <= radiusSquared)
    {
        return referencePos;
    }

    float const distance = sqrtf(distanceSquared);
    if (distance <= 0.f)
    {
        return sphereCenter;
    }

    Vec3 const outwardNormal = displacement / distance;
    return sphereCenter + outwardNormal * sphereRadius;
}

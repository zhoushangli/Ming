#include "Engine/Math/MathUtils.hpp"

#include <math.h>

float Max(float a, float b)
{
    return (a > b) ? a : b;
}

float Min(float a, float b)
{
    return (a < b) ? a : b;
}

float Abs(float a)
{
    return abs(a);
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

float GetDistance2D(Vec2 const& a, Vec2 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float GetDistanceSquared2D(Vec2 const& a, Vec2 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

float GetDistance3D(Vec3 const& a, Vec3 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

float GetDistanceXY3D(Vec3 const& a, Vec3 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float GetDistanceSquared3D(Vec3 const& a, Vec3 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

float GetDistanceXYSquared3D(Vec3 const& a, Vec3 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB)
{
    float distSquared = GetDistanceSquared2D(centerA, centerB);
    float radiiSum = radiusA + radiusB;
    return distSquared <= (radiiSum * radiiSum);
}

bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB)
{
    float distSquared = GetDistanceSquared3D(centerA, centerB);
    float radiiSum = radiusA + radiusB;
    return distSquared <= (radiiSum * radiiSum);
}

void TransformPosition2D(Vec2& pos, float scale, float rotationDegrees, Vec2 const& translation)
{
    pos *= scale;
	pos.RotateDegrees(rotationDegrees);
	pos += translation;
}

void TransformPosition2D(Vec2& pos, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
    float x = pos.x;
    float y = pos.y;
    pos = iBasis * x + jBasis * y + translation;
}

void TransformPositionXY3D(Vec3& pos, float scaleXY, float zRotationDegrees, Vec2 const& translationXY)
{
    pos.x *= scaleXY;
	pos.y *= scaleXY;
    pos = pos.GetRotatedAboutZDegrees(zRotationDegrees);
    pos.x += translationXY.x;
    pos.y += translationXY.y;
}

void TransformPositionXY3D(Vec3& pos, Vec2 const& iBasisXY, Vec2 const& jBasisXY, Vec2 const& translationXY)
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

float DotProduct2D(Vec2 const& a, Vec2 const& b)
{
	return a.x * b.x + a.y * b.y;
}

Vec2 GetNearestPointOnDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius)
{
    Vec2 toPoint = point - discCenter;
    float dist = toPoint.GetLength();

    if (dist <= discRadius || dist == 0.f) 
    {
        return point;
    }

    return discCenter + toPoint * (discRadius / dist);
}

bool PushDiscOutOfFixedPoint2D(Vec2& discCenter, float discRadius, Vec2 const& fixedPoint)
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

bool PushDiscOutOfFixedDisc2D(Vec2& discCenter, float discRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius)
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

bool PushDiscsOutOfEachOther2D(Vec2& discCenterA, float discRadiusA, Vec2& discCenterB, float discRadiusB)
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

bool PushDiscOutOfFixedAABB2D(Vec2& discCenter, float discRadius, AABB2 const& box)
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

float GetProjectedLength2D(Vec2 const& vector, Vec2 const& basis)
{
    Vec2 n = basis.GetNormalized();
    return DotProduct2D(vector, n);
}

Vec2 GetProjectedVector2D(Vec2 const& vector, Vec2 const& basis)
{
    Vec2 n = basis.GetNormalized();
    return n * DotProduct2D(vector, n);
}

float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b)
{
    float aLen = a.GetLength();
    float bLen = b.GetLength();
    if (aLen == 0.f || bLen == 0.f) return 0.f;
    float dot = DotProduct2D(a, b) / (aLen * bLen);
    dot = GetClamped(dot, -1.f, 1.f); 
    return ConvertRadiansToDegrees(acosf(dot));
}

int GetTaxicabDistance2D(IntVec2 const& a, IntVec2 const& b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorMaxRange) {
    Vec2 toPoint = point - sectorOrigin;
    float dist = toPoint.GetLength();
    if (dist > sectorMaxRange || dist == 0.f) return false;
    float pointDir = toPoint.GetOrientationDegrees();
    float delta = GetShortestAngularDispDegrees(sectorForwardDegrees, pointDir);
    return abs(delta) <= (sectorApertureDegrees * 0.5f);
}

bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorOrigin, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorMaxRange) {
    Vec2 toPoint = point - sectorOrigin;
    float dist = toPoint.GetLength();
    if (dist > sectorMaxRange || dist == 0.f) return false;
    float forwardDir = sectorForwardNormal.GetOrientationDegrees();
    float pointDir = toPoint.GetOrientationDegrees();
    float delta = GetShortestAngularDispDegrees(forwardDir, pointDir);
    return abs(delta) <= (sectorApertureDegrees * 0.5f);
}


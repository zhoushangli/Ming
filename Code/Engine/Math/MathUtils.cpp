#include "Engine/Math/MathUtils.hpp"

#include <math.h>

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

// --- Is Point Inside ---
bool IsPointInsideDisc2D(Vec2 point, Vec2 discCenter, float discRadius)
{
	Vec2 toPoint = point - discCenter;
	float distSquared = toPoint.GetLengthSquared();
	return distSquared < (discRadius * discRadius);
}

bool IsPointInsideDisc2D(Vec2 point, Disc2 const& disc)
{
	return IsPointInsideDisc2D(point, disc.m_center, disc.m_radius);
}

bool IsPointInsideAABB2D(Vec2 point, AABB2 const& alignedBox)
{
    return (point.x > alignedBox.m_mins.x && point.x < alignedBox.m_maxs.x &&
		point.y > alignedBox.m_mins.y && point.y < alignedBox.m_maxs.y);
}

bool IsPointInsideOBB2D(Vec2 point, OBB2 const& orientedBox)
{
	Vec2 localPos = orientedBox.GetLocalPosForWorldPos(point);
	return IsPointInsideAABB2D(localPos, AABB2(-orientedBox.m_halfDimensions, orientedBox.m_halfDimensions));
}

bool IsPointInsideCapsule2D(Vec2 point, Vec2 boneStart, Vec2 boneEnd, float radius)
{
	Vec2 nearestPoint = GetNearestPointOnLineSegment2D(point, boneStart, boneEnd);
	return IsPointInsideDisc2D(point, nearestPoint, radius);
}

bool IsPointInsideCapsule2D(Vec2 point, Capsule2 const& capsule)
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

	if (DotProduct2D(edge0Rotate90, toPoint0) < 0.f) return false;
	if (DotProduct2D(edge1Rotate90, toPoint1) < 0.f) return false;
	if (DotProduct2D(edge2Rotate90, toPoint2) < 0.f) return false;
	
    return true;
}

bool IsPointInsideTriangle2D(Vec2 point, Triangle2 const& triangle)
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

Vec2 GetNearestPointOnDisc2D(Vec2 referencePos, Disc2 const& disc)
{
	return GetNearestPointOnDisc2D(referencePos, disc.m_center, disc.m_radius);
}

Vec2 GetNearestPointOnAABB2D(Vec2 referencePos, AABB2 const& alignedBox)
{
	return Vec2(
		GetClamped(referencePos.x, alignedBox.m_mins.x, alignedBox.m_maxs.x),
		GetClamped(referencePos.y, alignedBox.m_mins.y, alignedBox.m_maxs.y)
	);
}

Vec2 GetNearestPointOnOBB2D(Vec2 referencePos, OBB2 const& orientedBox)
{
	Vec2 localPos = orientedBox.GetLocalPosForWorldPos(referencePos);
	Vec2 clampedLocalPos = Vec2(
		GetClamped(localPos.x, -orientedBox.m_halfDimensions.x, orientedBox.m_halfDimensions.x),
		GetClamped(localPos.y, -orientedBox.m_halfDimensions.y, orientedBox.m_halfDimensions.y)
	);
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

Vec2 GetNearestPointOnInfiniteLine2D(Vec2 referencePos, LineSegment2 const& lineSegmentOnInfiniteLine)
{
	return GetNearestPointOnInfiniteLine2D(referencePos, lineSegmentOnInfiniteLine.m_start, lineSegmentOnInfiniteLine.m_end);
}

Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, Vec2 start, Vec2 end)
{
	Vec2 startToEnd = end - start;
	Vec2 endToStart = start - end;

	Vec2 startToRef = referencePos - start;
	Vec2 endToRef = referencePos - end;

    if (DotProduct2D(startToRef, startToEnd) <= 0.f)    return start;
	if (DotProduct2D(endToRef, endToStart) <= 0.f)      return end;

	return GetNearestPointOnInfiniteLine2D(referencePos, start, end);
}

Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, LineSegment2 const& lineSegment)
{
	return GetNearestPointOnLineSegment2D(referencePos, lineSegment.m_start, lineSegment.m_end);
}

Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Vec2 boneStart, Vec2 boneEnd, float radius)
{
	Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(referencePos, boneStart, boneEnd);
	return GetNearestPointOnDisc2D(referencePos, nearestPointOnBone, radius);
}

Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Capsule2 const& capsule)
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

Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Triangle2 const& triangle)
{
	return GetNearestPointOnTriangle2D(referencePos, triangle.m_pointsCounterClockwise[0], triangle.m_pointsCounterClockwise[1], triangle.m_pointsCounterClockwise[2]);
}

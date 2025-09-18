#include "MathUtils.hpp"

#include <cmath>

#include "MathCommon.hpp"

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
    return std::cos(ConvertDegreesToRadians(degrees));
}

float SinDegrees(float degrees)
{
    return std::sin(ConvertDegreesToRadians(degrees));
}

float Atan2Degrees(float y, float x)
{
    return ConvertRadiansToDegrees(std::atan2(y, x));
}

float GetDistance2D(Vec2 const& a, Vec2 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
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
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float GetDistanceXY3D(Vec3 const& a, Vec3 const& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
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

Vec2 TransformPosition2D(Vec2& pos, float scale, float rotationDegrees, Vec2 const& translation)
{
    pos *= scale;
	pos.RotateDegrees(rotationDegrees);
	pos += translation;
    return pos;
}

Vec3 TransformPositionXY3D(Vec3& pos, float scaleXY, float zRotationDegrees, Vec2 const& translationXY)
{
    pos.x *= scaleXY;
	pos.y *= scaleXY;
    pos = pos.GetRotatedAboutZDegrees(zRotationDegrees);
    pos.x += translationXY.x;
    pos.y += translationXY.y;
    return pos;
}

float Interpolate(float start, float end, float fraction)
{
	return start * (1.0f - fraction) + end * fraction;
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


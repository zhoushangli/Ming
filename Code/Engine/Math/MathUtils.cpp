#include "MathUtils.hpp"
#include <cmath>

constexpr float PI = 3.14159265358979323846f;
constexpr float RadiansToDegreesMultiplier = 57.29577951f;
constexpr float DegreesToRadiansMultiplier = 0.01745329252f;

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


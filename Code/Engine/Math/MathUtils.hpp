#pragma once

#include "Vec2.hpp"
#include "Vec3.hpp"

// Angle conversion and trigonometric functions
float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float radians);
float CosDegrees(float degrees);
float SinDegrees(float degrees);
float Atan2Degrees(float y, float x);

// Distance and overlap functions
float GetDistance2D(Vec2 const& a, Vec2 const& b);
float GetDistanceSquared2D(Vec2 const& a, Vec2 const& b);

float GetDistance3D(Vec3 const& a, Vec3 const& b);
float GetDistanceXY3D(Vec3 const& a, Vec3 const& b);
float GetDistanceSquared3D(Vec3 const& a, Vec3 const& b);
float GetDistanceXYSquared3D(Vec3 const& a, Vec3 const& b);

bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);

// Transformations
Vec2 TransformPosition2D(Vec2& pos, float scale, float rotationDegrees, Vec2 const& translation);
Vec3 TransformPositionXY3D(Vec3& pos, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);

// Interpolation and range mapping functions
float Interpolate(float start, float end, float fraction);
float GetFractionWithinRange(float value, float start, float end);
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float GetClamped(float value, float minValue, float maxValue);
float GetClampedZeroToOne(float value);
int RoundDownToInt(float value);

// Angular displacement and dot product functions
float GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);
float DotProduct2D(Vec2 const& a, Vec2 const& b);
#pragma once

#include "Vec2.hpp"
#include "Vec3.hpp"
#include "AABB2.hpp"
#include "IntVec2.hpp"

float const DegreesToRadiansMultiplier  = 0.017453292519943295f;    // PI / 180
float const RadiansToDegreesMultiplier  = 57.29577951308232f;       // 180 / PI
const float PI                          = 3.14159265358979323846f;
const float TWO_PI                      = 6.28318530717958647692f;
const float HALF_PI                     = 1.57079632679489661923f;

float   Max(float a, float b);
float   Min(float a, float b);
float   Abs(float a);

// Angle conversion and trigonometric functions
float	ConvertDegreesToRadians(float degrees);
float	ConvertRadiansToDegrees(float radians);
float	CosDegrees(float degrees);
float	SinDegrees(float degrees);
float	Atan2Degrees(float y, float x);

// Distance and overlap functions
float	GetDistance2D(Vec2 const& a, Vec2 const& b);
float	GetDistanceSquared2D(Vec2 const& a, Vec2 const& b);

float	GetDistance3D(Vec3 const& a, Vec3 const& b);
float	GetDistanceXY3D(Vec3 const& a, Vec3 const& b);
float	GetDistanceSquared3D(Vec3 const& a, Vec3 const& b);
float	GetDistanceXYSquared3D(Vec3 const& a, Vec3 const& b);

bool	DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool	DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);

// Transformations
void	TransformPosition2D(Vec2& pos, float scale, float rotationDegrees, Vec2 const& translation);
void	TransformPosition2D(Vec2& pos, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void	TransformPositionXY3D(Vec3& pos, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void	TransformPositionXY3D(Vec3& pos, Vec2 const& iBasisXY, Vec2 const& jBasisXY, Vec2 const& translationXY);

// Interpolation and range mapping functions 
float	Interpolate(float start, float end, float fraction);
float   InterpolateClamped(float start, float end, float fraction);
float	GetFractionWithinRange(float value, float start, float end);
float	RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float	RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float	GetClamped(float value, float minValue, float maxValue);
float	GetClampedZeroToOne(float value);
int		RoundDownToInt(float value);

// Angular displacement and dot product functions
float	GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float	GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);
float	DotProduct2D(Vec2 const& vector, Vec2 const& basis);

// Geometric utilities
Vec2	GetNearestPointOnDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius);
bool	PushDiscOutOfFixedPoint2D(Vec2& discCenter, float discRadius, Vec2 const& fixedPoint);
bool	PushDiscOutOfFixedDisc2D(Vec2& discCenter, float discRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool	PushDiscsOutOfEachOther2D(Vec2& discCenterA, float discRadiusA, Vec2& discCenterB, float discRadiusB);
bool	PushDiscOutOfFixedAABB2D(Vec2& discCenter, float discRadius, AABB2 const& box);

// Projection and angle functions
float	GetProjectedLength2D(Vec2 const& vector, Vec2 const& basis);
Vec2	GetProjectedVector2D(Vec2 const& vector, Vec2 const& basis);
float	GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b);

// Miscellaneous utilities
int		GetTaxicabDistance2D(IntVec2 const& a, IntVec2 const& b);
bool	IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorMaxRange);
bool	IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorOrigin, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorMaxRange);


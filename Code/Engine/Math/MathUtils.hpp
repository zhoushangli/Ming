#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/Disc2.hpp"

float const DegreesToRadiansMultiplier  = 0.017453292519943295f;    // PI / 180
float const RadiansToDegreesMultiplier  = 57.29577951308232f;       // 180 / PI
const float PI                          = 3.14159265358979323846f;
const float TWO_PI                      = 6.28318530717958647692f;
const float HALF_PI                     = 1.57079632679489661923f;

float Abs(float value);
float Sign(float value);
float Min(float a, float b);
int Min(int a, int b);
float Max(float a, float b);
int Max(int a, int b);

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
void TransformPosition2D(Vec2& pos, float scale, float rotationDegrees, Vec2 const& translation);
void TransformPosition2D(Vec2& pos, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPositionXY3D(Vec3& pos, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void TransformPositionXY3D(Vec3& pos, Vec2 const& iBasisXY, Vec2 const& jBasisXY, Vec2 const& translationXY);

// Interpolation and range mapping functions 
float Interpolate(float start, float end, float fraction);
float InterpolateClamped(float start, float end, float fraction);
float GetFractionWithinRange(float value, float start, float end);
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float GetClamped(float value, float minValue, float maxValue);
float GetClampedZeroToOne(float value);
int	 RoundDownToInt(float value);

// Angular displacement and dot product functions
float GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);
float DotProduct2D(Vec2 const& vector, Vec2 const& basis);

// Geometric utilities
bool PushDiscOutOfFixedPoint2D(Vec2& discCenter, float discRadius, Vec2 const& fixedPoint);
bool PushDiscOutOfFixedDisc2D(Vec2& discCenter, float discRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscsOutOfEachOther2D(Vec2& discCenterA, float discRadiusA, Vec2& discCenterB, float discRadiusB);
bool PushDiscOutOfFixedAABB2D(Vec2& discCenter, float discRadius, AABB2 const& box);

// Projection and angle functions
float GetProjectedLength2D(Vec2 const& vector, Vec2 const& basis);
Vec2 GetProjectedVector2D(Vec2 const& vector, Vec2 const& basis);
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b);

// Miscellaneous utilities
int	GetTaxicabDistance2D(IntVec2 const& a, IntVec2 const& b);
int GetTaxicabDistance2D(Vec2 const& a, Vec2 const& b);

// --- Is Point Inside ---
bool IsPointInsideDisc2D(Vec2 point, Vec2 discCenter, float discRadius);
bool IsPointInsideDisc2D(Vec2 point, Disc2 const& disc);
bool IsPointInsideAABB2D(Vec2 point, AABB2 const& alignedBox);
bool IsPointInsideOBB2D(Vec2 point, OBB2 const& orientedBox);
bool IsPointInsideCapsule2D(Vec2 point, Vec2 boneStart, Vec2 boneEnd, float radius);
bool IsPointInsideCapsule2D(Vec2 point, Capsule2 const& capsule);
bool IsPointInsideTriangle2D(Vec2 point, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2); // Counter-Clockwise (positive winding)
bool IsPointInsideTriangle2D(Vec2 point, Triangle2 const& triangle);
bool IsPointInsideOrientedSector2D(Vec2 point, Vec2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 point, Vec2 sectorOrigin, Vec2 sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);

// --- Get Nearest Point On ---
Vec2 GetNearestPointOnDisc2D(Vec2 referencePos, Vec2 discCenter, float discRadius);
Vec2 GetNearestPointOnDisc2D(Vec2 referencePos, Disc2 const& disc);
Vec2 GetNearestPointOnAABB2D(Vec2 referencePos, AABB2 const& alignedBox);
Vec2 GetNearestPointOnOBB2D(Vec2 referencePos, OBB2 const& orientedBox);
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 referencePos, Vec2 pointOnLine, Vec2 anotherPointOnLine);
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 referencePos, LineSegment2 const& lineSegmentOnInfiniteLine);
Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, Vec2 start, Vec2 end);
Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, LineSegment2 const& lineSegment);
Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Vec2 boneStart, Vec2 boneEnd, float radius);
Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Capsule2 const& capsule);
Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2); // Counter-Clockwise (positive winding)
Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Triangle2 const& triangle);

#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/Sphere3.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Math/CylinderZ3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "AABB3.hpp"

//------------------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------------------
#pragma region Constants

float const DegreesToRadiansMultiplier = 0.017453292519943295f; // PI / 180
float const RadiansToDegreesMultiplier = 57.29577951308232f;    // 180 / PI
const float PI = 3.14159265358979323846f;
const float TWO_PI = 6.28318530717958647692f;
const float HALF_PI = 1.57079632679489661923f;

#pragma endregion

//------------------------------------------------------------------------------------------------
// Basic math helpers
//------------------------------------------------------------------------------------------------
#pragma region Basic Math Helpers

float Abs(float value);
float Sign(float value);
float Min(float a, float b);
int Min(int a, int b);
float Max(float a, float b);
int Max(int a, int b);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Angles & Trigonometry
//------------------------------------------------------------------------------------------------
#pragma region Angles & Trigonometry

float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float radians);
float CosDegrees(float degrees);
float SinDegrees(float degrees);
float Atan2Degrees(float y, float x);

float GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Distance & Overlap Tests
//------------------------------------------------------------------------------------------------
#pragma region Distance & Overlap Tests

float GetDistance2D(Vec2 const &a, Vec2 const &b);
float GetDistanceSquared2D(Vec2 const &a, Vec2 const &b);

bool DoDiscsOverlap2D(Vec2 const &centerA, float radiusA, Vec2 const &centerB, float radiusB);

bool DoAABB3sOverlap3D(Vec3 const &firstMins, Vec3 const &firstMaxs, Vec3 const &secondMins, Vec3 const &secondMaxs);
bool DoAABB3sOverlap3D(AABB3 const &first, AABB3 const &second);

float GetDistance3D(Vec3 const &a, Vec3 const &b);
float GetDistanceXY3D(Vec3 const &a, Vec3 const &b);
float GetDistanceSquared3D(Vec3 const &a, Vec3 const &b);
float GetDistanceXYSquared3D(Vec3 const &a, Vec3 const &b);

bool DoSpheresOverlap3D(Vec3 const &centerA, float radiusA, Vec3 const &centerB, float radiusB);
bool DoSpheresOverlap3D(Sphere3 const &a, Sphere3 const &b);

bool DoCylinderZsOverlap3D(Vec2 const &cylinder1CenterXY, float cylinder1Radius, FloatRange const &cylinder1MinMaxZ,
                           Vec2 const &cylinder2CenterXY, float cylinder2Radius, FloatRange const &cylinder2MinMaxZ);
bool DoCylinderZsOverlap3D(Vec3 const &centerA, float radiusA, float heightA, Vec3 const &centerB, float radiusB, float heightB);

bool DoSphereAndAABBOverlap3D(Vec3 const &sphereCenter, float sphereRadius, Vec3 const &boxMins, Vec3 const &boxMaxs);
bool DoSphereAndAABBOverlap3D(Vec3 const &sphereCenter, float sphereRadius, AABB3 const &box);
bool DoSphereAndAABBOverlap3D(Sphere3 const &sphere, AABB3 const &box);

bool DoCylinderZAndAABBOverlap3D(Vec2 const &cylinderCenterXY, float cylinderRadius, FloatRange const &cylinderMinMaxZ, Vec3 const &boxMins, Vec3 const &boxMaxs);
bool DoCylinderZAndAABBOverlap3D(Vec2 const &cylinderCenterXY, float cylinderRadius, FloatRange const &cylinderMinMaxZ, AABB3 const &box);
bool DoCylinderZAndAABBOverlap3D(CylinderZ3 const &cylinder, AABB3 const &box);

bool DoCylinderZAndSphereOverlap3D(Vec2 const &cylinderCenterXY, float cylinderRadius, FloatRange const &cylinderMinMaxZ, Vec3 const &sphereCenter, float sphereRadius);
bool DoCylinderZAndSphereOverlap3D(CylinderZ3 const &cylinder, Sphere3 const &sphere);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Transforms (in-place helpers)
//------------------------------------------------------------------------------------------------
#pragma region Transforms

void TransformPosition2D(Vec2 &pos, float scale, float rotationDegrees, Vec2 const &translation);
void TransformPosition2D(Vec2 &pos, Vec2 const &iBasis, Vec2 const &jBasis, Vec2 const &translation);
void TransformPositionXY3D(Vec3 &pos, float scaleXY, float zRotationDegrees, Vec2 const &translationXY);
void TransformPositionXY3D(Vec3 &pos, Vec2 const &iBasisXY, Vec2 const &jBasisXY, Vec2 const &translationXY);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Interpolation, Mapping, Clamping
//------------------------------------------------------------------------------------------------
#pragma region Interpolation / Mapping / Clamping

float Interpolate(float start, float end, float fraction);
float InterpolateClamped(float start, float end, float fraction);
float GetFractionWithinRange(float value, float start, float end);
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);

int GetClamped(int value, int minValue, int maxValue);
float GetClamped(float value, float minValue, float maxValue);
float GetClampedZeroToOne(float value);
int RoundDownToInt(float value);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Dot & Cross Products / Projections / Vector-Angle Helpers
//------------------------------------------------------------------------------------------------
#pragma region Dot / Cross / Projection

float DotProduct2D(Vec2 const &vector, Vec2 const &basis);
float DotProduct2D(Vec2 const &a, Vec2 const &b);
float DotProduct3D(Vec3 const &a, Vec3 const &b);
float DotProduct4D(Vec4 const &a, Vec4 const &b);

float CrossProduct2D(Vec2 const &a, Vec2 const &b);
Vec3 CrossProduct3D(Vec3 const &a, Vec3 const &b);

float GetProjectedLength2D(Vec2 const &vector, Vec2 const &basis);
Vec2 GetProjectedVector2D(Vec2 const &vector, Vec2 const &basis);
Vec3 GetProjectedVector3D(Vec3 const &vector, Vec3 const &basis);
float GetAngleDegreesBetweenVectors2D(Vec2 const &a, Vec2 const &b);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry: Push Out (collision resolution helpers)
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Push Out

bool PushDiscOutOfFixedPoint2D(Vec2 &discCenter, float discRadius, Vec2 const &fixedPoint);
bool PushDiscOutOfFixedDisc2D(Vec2 &discCenter, float discRadius, Vec2 const &fixedDiscCenter, float fixedDiscRadius);
bool PushDiscOutOfFixedDisc2D(Disc2 &discToPush, Disc2 const &fixedDisc);
bool PushDiscsOutOfEachOther2D(Vec2 &discCenterA, float discRadiusA, Vec2 &discCenterB, float discRadiusB);
bool PushDiscsOutOfEachOther2D(Disc2 &discA, Disc2 &discB);
bool PushDiscOutOfFixedAABB2D(Vec2 &discCenter, float discRadius, AABB2 const &box);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry Queries: Point Inside Tests
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Point Inside

bool IsPointInsideDisc2D(Vec2 point, Vec2 discCenter, float discRadius);
bool IsPointInsideDisc2D(Vec2 point, Disc2 const &disc);
bool IsPointInsideAABB2D(Vec2 point, AABB2 const &alignedBox);
bool IsPointInsideOBB2D(Vec2 point, OBB2 const &orientedBox);
bool IsPointInsideCapsule2D(Vec2 point, Vec2 boneStart, Vec2 boneEnd, float radius);
bool IsPointInsideCapsule2D(Vec2 point, Capsule2 const &capsule);
bool IsPointInsideTriangle2D(Vec2 point, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2); // Counter-Clockwise (positive winding)
bool IsPointInsideTriangle2D(Vec2 point, Triangle2 const &triangle);
bool IsPointInsideOrientedSector2D(Vec2 point, Vec2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 point, Vec2 sectorOrigin, Vec2 sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry Queries: Nearest Point
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Nearest Point

Vec2 GetNearestPointOnDisc2D(Vec2 referencePos, Vec2 discCenter, float discRadius);
Vec2 GetNearestPointOnDisc2D(Vec2 referencePos, Disc2 const &disc);
Vec2 GetNearestPointOnAABB2D(Vec2 referencePos, AABB2 const &alignedBox);
Vec2 GetNearestPointOnOBB2D(Vec2 referencePos, OBB2 const &orientedBox);
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 referencePos, Vec2 pointOnLine, Vec2 anotherPointOnLine);
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 referencePos, LineSegment2 const &lineSegmentOnInfiniteLine);
Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, Vec2 start, Vec2 end);
Vec2 GetNearestPointOnLineSegment2D(Vec2 referencePos, LineSegment2 const &lineSegment);
Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Vec2 boneStart, Vec2 boneEnd, float radius);
Vec2 GetNearestPointOnCapsule2D(Vec2 referencePos, Capsule2 const &capsule);
Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2); // Counter-Clockwise (positive winding)
Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Triangle2 const &triangle);
Vec3 GetNearestPointOnAABB3D(Vec3 referencePos, AABB3 const &alignedBox);
Vec3 GetNearestPointOnAABB3D(Vec3 referencePos, Vec3 const &boxMins, Vec3 const &boxMaxs);
Vec3 GetNearestPointOnZCylinder3D(Vec3 referencePos, CylinderZ3 const &cylinder);
Vec3 GetNearestPointOnZCylinder3D(Vec3 referencePos, Vec3 const &cylinderStart, float cylinderHeight, float cylinderRadius);
Vec3 GetNearestPointOnSphere3D(Vec3 referencePos, Sphere3 const &sphere);
Vec3 GetNearestPointOnSphere3D(Vec3 referencePos, Vec3 const &sphereCenter, float sphereRadius);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Misc / Integer grid helpers
//------------------------------------------------------------------------------------------------
#pragma region Misc

int GetTaxicabDistance2D(IntVec2 const &a, IntVec2 const &b);
int GetTaxicabDistance2D(Vec2 const &a, Vec2 const &b);

float NormalizeByte(unsigned char byteValue);
unsigned char DenormalizeByte(float zeroToOne);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Billboarding
//------------------------------------------------------------------------------------------------
#pragma region Billboarding

enum class BillboardType
{
    NONE = -1,
    WORLD_UP_FACING,
    WORLD_UP_OPPOSING,
    FULL_FACING,
    FULL_OPPOSING,
    COUNT
};

Matrix4x4 GetBillboardTransform(
    BillboardType billboardType,
    Matrix4x4 const &targetTransform,
    const Vec3 &billboardPosition,
    const Vec2 &billboardScale = Vec2(1.0f, 1.0f));

#pragma endregion
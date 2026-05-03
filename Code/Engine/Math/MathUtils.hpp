#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Capsule3.hpp"
#include "Engine/Math/CylinderZ3.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Sphere3.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"

//------------------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------------------
#pragma region Constants

float const DegreesToRadiansMultiplier = 0.017453292519943295f; // PI / 180
float const RadiansToDegreesMultiplier = 57.29577951308232f;    // 180 / PI
const float PI                         = 3.14159265358979323846f;
const float TWO_PI                     = 6.28318530717958647692f;
const float HALF_PI                    = 1.57079632679489661923f;

#pragma endregion

//------------------------------------------------------------------------------------------------
// Basic math helpers
//------------------------------------------------------------------------------------------------
#pragma region Basic Math Helpers

float Abs(float value);
float Sign(float value);
float Min(float a, float b);
int   Min(int a, int b);
float Max(float a, float b);
int   Max(int a, int b);

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

float GetDistance2D(Vec2 const& a, Vec2 const& b);
float GetDistanceSquared2D(Vec2 const& a, Vec2 const& b);

bool DoDiscsOverlap2D(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoDiscsOverlap2D(Disc2 const& discA, Disc2 const& discB);
bool DoDiscAndLineOverlap2D(Vec2 const& discCenter, float discRadius, Vec2 const& lineStart, Vec2 const& lineEnd);
bool DoDiscAndLineOverlap2D(Disc2 const& disc, LineSegment2 const& line);
bool DoDiscAndAABBOverlap2D(Vec2 const& discCenter, float discRadius, Vec2 const& boxMins, Vec2 const& boxMaxs);
bool DoDiscAndAABBOverlap2D(Vec2 const& discCenter, float discRadius, AABB2 const& box);
bool DoDiscAndAABBOverlap2D(Disc2 const& disc, AABB2 const& box);
bool DoDiscAndCapsuleOverlap2D(Vec2 const& discCenter, float discRadius, Vec2 const& capsuleStart, Vec2 const& capsuleEnd, float capsuleRadius);
bool DoDiscAndCapsuleOverlap2D(Disc2 const& disc, Capsule2 const& capsule);
bool DoDiscAndOBBOverlap2D(Vec2 const& discCenter, float discRadius, OBB2 const& box);
bool DoDiscAndOBBOverlap2D(Disc2 const& disc, OBB2 const& box);

bool DoAABB3sOverlap3D(Vec3 const& firstMins, Vec3 const& firstMaxs, Vec3 const& secondMins, Vec3 const& secondMaxs);
bool DoAABB3sOverlap3D(AABB3 const& first, AABB3 const& second);

float GetDistance3D(Vec3 const& a, Vec3 const& b);
float GetDistanceXY3D(Vec3 const& a, Vec3 const& b);
float GetDistanceSquared3D(Vec3 const& a, Vec3 const& b);
float GetDistanceXYSquared3D(Vec3 const& a, Vec3 const& b);

bool DoSpheresOverlap3D(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool DoSpheresOverlap3D(Sphere3 const& a, Sphere3 const& b);

bool DoCylinderZsOverlap3D(
	Vec2 const&       cylinder1CenterXY,
	float             cylinder1Radius,
	FloatRange const& cylinder1MinMaxZ,
	Vec2 const&       cylinder2CenterXY,
	float             cylinder2Radius,
	FloatRange const& cylinder2MinMaxZ
);
bool DoCylinderZsOverlap3D(
	Vec3 const& centerA, float radiusA, float heightA, Vec3 const& centerB, float radiusB, float heightB
);

bool DoSphereAndAABBOverlap3D(Vec3 const& sphereCenter, float sphereRadius, Vec3 const& boxMins, Vec3 const& boxMaxs);
bool DoSphereAndAABBOverlap3D(Vec3 const& sphereCenter, float sphereRadius, AABB3 const& box);
bool DoSphereAndAABBOverlap3D(Sphere3 const& sphere, AABB3 const& box);

bool DoCylinderZAndAABBOverlap3D(
	Vec2 const&       cylinderCenterXY,
	float             cylinderRadius,
	FloatRange const& cylinderMinMaxZ,
	Vec3 const&       boxMins,
	Vec3 const&       boxMaxs
);
bool DoCylinderZAndAABBOverlap3D(
	Vec2 const& cylinderCenterXY, float cylinderRadius, FloatRange const& cylinderMinMaxZ, AABB3 const& box
);
bool DoCylinderZAndAABBOverlap3D(CylinderZ3 const& cylinder, AABB3 const& box);

bool DoCylinderZAndSphereOverlap3D(
	Vec2 const&       cylinderCenterXY,
	float             cylinderRadius,
	FloatRange const& cylinderMinMaxZ,
	Vec3 const&       sphereCenter,
	float             sphereRadius
);
bool DoCylinderZAndSphereOverlap3D(CylinderZ3 const& cylinder, Sphere3 const& sphere);

bool DoCapsulesOverlap3D(Capsule3 const& capsuleA, Capsule3 const& capsuleB);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Transforms (in-place helpers)
//------------------------------------------------------------------------------------------------
#pragma region Transforms

void TransformPosition2D(Vec2& pos, float scale, float rotationDegrees, Vec2 const& translation);
void TransformPosition2D(Vec2& pos, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPositionXY3D(Vec3& pos, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void TransformPositionXY3D(Vec3& pos, Vec2 const& iBasisXY, Vec2 const& jBasisXY, Vec2 const& translationXY);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Interpolation, Mapping, Clamping
//------------------------------------------------------------------------------------------------
#pragma region Interpolation / Mapping / Clamping

float       Interpolate(float start, float end, float fraction);
Vec3        Interpolate(Vec3 const& start, Vec3 const& end, float fraction);
Rgba8       Interpolate(Rgba8 const& start, Rgba8 const& end, float fraction);
EulerAngles Interpolate(EulerAngles const& start, EulerAngles const& end, float fraction);

float       InterpolateClamped(float start, float end, float fraction);
Vec3        InterpolateClamped(Vec3 const& start, Vec3 const& end, float fraction);
Rgba8       InterpolateClamped(Rgba8 const& start, Rgba8 const& end, float fraction);
EulerAngles InterpolateClamped(EulerAngles const& start, EulerAngles const& end, float fraction);

float SmoothStart2(float t);
float SmoothStart3(float t);
float SmoothStart4(float t);
float SmoothStart5(float t);
float SmoothStart6(float t);

float SmoothStop2(float t);
float SmoothStop3(float t);
float SmoothStop4(float t);
float SmoothStop5(float t);
float SmoothStop6(float t);

float SmoothStep3(float t);
float SmoothStep5(float t);
float Hesitate3(float t);
float Hesitate5(float t);

float String(float t);
float String(float t, float frequency, float decay);

float ComputeCubicBezier1D(float A, float B, float C, float D, float t);
float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t);

Vec2 ComputeCubicBezier2D(Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, float t);
Vec2 ComputeQuinticBezier2D(
	Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, Vec2 const& E, Vec2 const& F, float t
);

Vec3 ComputeCubicBezier3D(Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, float t);
Vec3 ComputeQuinticBezier3D(
	Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, Vec3 const& E, Vec3 const& F, float t
);

float GetFractionWithinRange(float value, float start, float end);
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);

int   GetClamped(int value, int minValue, int maxValue);
float GetClamped(float value, float minValue, float maxValue);
float GetClampedZeroToOne(float value);
int   RoundDownToInt(float value);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Dot & Cross Products / Projections / Vector-Angle Helpers
//------------------------------------------------------------------------------------------------
#pragma region Dot / Cross / Projection

float DotProduct2D(Vec2 const& vector, Vec2 const& basis);
float DotProduct2D(Vec2 const& a, Vec2 const& b);
float DotProduct3D(Vec3 const& a, Vec3 const& b);
float DotProduct4D(Vec4 const& a, Vec4 const& b);

// About the cross product direction
// 1) Show your right hand!
// 2) Point at the first vector with your index finger
// 3) Point at the second vector with your middle finger
// 4) Your thumb will now be pointing in the direction of the cross product result
float CrossProduct2D(Vec2 const& a, Vec2 const& b);
Vec3  CrossProduct3D(Vec3 const& a, Vec3 const& b);

float GetProjectedLength2D(Vec2 const& vector, Vec2 const& basis);
Vec2  GetProjectedVector2D(Vec2 const& vector, Vec2 const& basis);
Vec3  GetProjectedVector3D(Vec3 const& vector, Vec3 const& basis);
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry: Push Out (collision resolution helpers)
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Push Out

bool PushDiscOutOfFixedPoint2D(Vec2& discCenter, float discRadius, Vec2 const& fixedPoint);
bool PushDiscOutOfFixedDisc2D(Vec2& discCenter, float discRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscOutOfFixedDisc2D(Disc2& discToPush, Disc2 const& fixedDisc);
bool PushDiscsOutOfEachOther2D(Vec2& discCenterA, float discRadiusA, Vec2& discCenterB, float discRadiusB);
bool PushDiscsOutOfEachOther2D(Disc2& discA, Disc2& discB);
bool PushDiscOutOfFixedAABB2D(Vec2& discCenter, float discRadius, AABB2 const& box);
bool PushDiscOutOfFixedAABB2D(Disc2& discToPush, AABB2 const& box);
bool PushDiscOutOfFixedLine2D(Vec2& discCenter, float discRadius, Vec2 const& lineStart, Vec2 const& lineEnd);
bool PushDiscOutOfFixedLine2D(Disc2& discToPush, LineSegment2 const& line);
bool PushDiscOutOfFixedCapsule2D(Vec2& discCenter, float discRadius, Vec2 const& capsuleStart, Vec2 const& capsuleEnd, float capsuleRadius);
bool PushDiscOutOfFixedCapsule2D(Disc2& discToPush, Capsule2 const& capsule);
bool PushDiscOutOfFixedOBB2D(Vec2& discCenter, float discRadius, OBB2 const& box);
bool PushDiscOutOfFixedOBB2D(Disc2& discToPush, OBB2 const& box);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry Queries: Point Inside Tests
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Point Inside

bool IsPointInsideDisc2D(Vec2 point, Vec2 discCenter, float discRadius);
bool IsPointInsideDisc2D(Vec2 point, Disc2 const& disc);
bool IsPointInsideAABB2D(Vec2 point, AABB2 const& alignedBox);
bool IsPointInsideAABB3D(Vec3 point, Vec3 const& boxMins, Vec3 const& boxMaxs);
bool IsPointInsideAABB3D(Vec3 point, AABB3 const& alignedBox);
bool IsPointInsideOBB2D(Vec2 point, OBB2 const& orientedBox);
bool IsPointInsideCapsule2D(Vec2 point, Vec2 boneStart, Vec2 boneEnd, float radius);
bool IsPointInsideCapsule2D(Vec2 point, Capsule2 const& capsule);
bool IsPointInsideTriangle2D(Vec2 point, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2); // Counter-Clockwise (positive winding)
bool IsPointInsideTriangle2D(Vec2 point, Triangle2 const& triangle);
bool IsPointInsideOrientedSector2D(
	Vec2 point, Vec2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius
);
bool IsPointInsideDirectedSector2D(
	Vec2 point, Vec2 sectorOrigin, Vec2 sectorForwardNormal, float sectorApertureDegrees, float sectorRadius
);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry Queries: Nearest Point
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Nearest Point

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
Vec2 GetNearestPointOnTriangle2D(
	Vec2 referencePos, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2
); // Counter-Clockwise (positive winding)
Vec2 GetNearestPointOnTriangle2D(Vec2 referencePos, Triangle2 const& triangle);
Vec3 GetNearestPointOnAABB3D(Vec3 referencePos, AABB3 const& alignedBox);
Vec3 GetNearestPointOnAABB3D(Vec3 referencePos, Vec3 const& boxMins, Vec3 const& boxMaxs);
Vec3 GetNearestPointOnZCylinder3D(Vec3 referencePos, CylinderZ3 const& cylinder);
Vec3 GetNearestPointOnZCylinder3D(
	Vec3 referencePos, Vec3 const& cylinderStart, float cylinderHeight, float cylinderRadius
);
Vec3 GetNearestPointOnSphere3D(Vec3 referencePos, Sphere3 const& sphere);
Vec3 GetNearestPointOnSphere3D(Vec3 referencePos, Vec3 const& sphereCenter, float sphereRadius);

Vec3 GetNearestPointOnLine3D(Vec3 referencePos, Vec3 const& lineStart, Vec3 const& lineEnd);
Vec3 GetNearestPointOnCapsule3D(Vec3 referencePos, Capsule3 const& capsule);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Misc / Integer grid helpers
//------------------------------------------------------------------------------------------------
#pragma region Misc

int GetTaxicabDistance2D(IntVec2 const& a, IntVec2 const& b);
int GetTaxicabDistance2D(Vec2 const& a, Vec2 const& b);

float         NormalizeByte(unsigned char byteValue);
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
	BillboardType    billboardType,
	Matrix4x4 const& targetTransform,
	const Vec3&      billboardPosition = Vec3::ZERO,
	const Vec2&      billboardScale    = Vec2::ONE
);

#pragma endregion

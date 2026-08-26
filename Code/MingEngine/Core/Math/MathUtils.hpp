#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/Capsule2.hpp"
#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/CylinderZ3.hpp"
#include "MingEngine/Core/Math/Disc2.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/FloatRange.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/OBB2.hpp"
#include "MingEngine/Core/Math/Sphere3.hpp"
#include "MingEngine/Core/Math/Triangle2.hpp"
#include "MingEngine/Core/Math/Triangle3.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/Math/Vector4.hpp"
#include "MingEngine/Core/Render/Color.hpp"

namespace Math
{

//------------------------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------------------------
#pragma region Constants

float const kDegreesToRadiansMultiplier = 0.017453292519943295f; // kPi / 180
float const kRadiansToDegreesMultiplier = 57.29577951308232f;    // 180 / kPi
const float kPi                         = 3.14159265358979323846f;
const float kTwoPi                      = 6.28318530717958647692f;
const float kHalfPi                     = 1.57079632679489661923f;

#pragma endregion

//------------------------------------------------------------------------------------------------
// Basic math helpers
//------------------------------------------------------------------------------------------------
#pragma region Basic Math Helpers

float Abs(float value);
int   Abs(int value);
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

float GetDistance2D(Vector2 const& a, Vector2 const& b);
float GetDistanceSquared2D(Vector2 const& a, Vector2 const& b);

bool DoDiscsOverlap2D(Vector2 const& centerA, float radiusA, Vector2 const& centerB, float radiusB);
bool DoDiscsOverlap2D(Disc2 const& discA, Disc2 const& discB);
bool DoDiscAndInfiniteLineOverlap2D(
	Vector2 const& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd);
bool DoDiscAndInfiniteLineOverlap2D(Disc2 const& disc, LineSegment2 const& line);
bool DoDiscAndLineOverlap2D(
	Vector2 const& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd);
bool DoDiscAndLineOverlap2D(Disc2 const& disc, LineSegment2 const& line);
bool DoDiscAndAABBOverlap2D(
	Vector2 const& discCenter, float discRadius, Vector2 const& boxMins, Vector2 const& boxMaxs);
bool DoDiscAndAABBOverlap2D(Vector2 const& discCenter, float discRadius, AABB2 const& box);
bool DoDiscAndAABBOverlap2D(Disc2 const& disc, AABB2 const& box);
bool DoDiscAndCapsuleOverlap2D(
	Vector2 const& discCenter,
	float          discRadius,
	Vector2 const& capsuleStart,
	Vector2 const& capsuleEnd,
	float          capsuleRadius);
bool DoDiscAndCapsuleOverlap2D(Disc2 const& disc, Capsule2 const& capsule);
bool DoDiscAndOBBOverlap2D(Vector2 const& discCenter, float discRadius, OBB2 const& box);
bool DoDiscAndOBBOverlap2D(Disc2 const& disc, OBB2 const& box);

bool DoAABB3sOverlap3D(
	Vector3 const& firstMins, Vector3 const& firstMaxs, Vector3 const& secondMins, Vector3 const& secondMaxs);
bool DoAABB3sOverlap3D(AABB3 const& first, AABB3 const& second);

float GetDistance3D(Vector3 const& a, Vector3 const& b);
float GetDistanceXY3D(Vector3 const& a, Vector3 const& b);
float GetDistanceSquared3D(Vector3 const& a, Vector3 const& b);
float GetDistanceXYSquared3D(Vector3 const& a, Vector3 const& b);

bool DoSpheresOverlap3D(Vector3 const& centerA, float radiusA, Vector3 const& centerB, float radiusB);
bool DoSpheresOverlap3D(Sphere3 const& a, Sphere3 const& b);

bool DoCylinderZsOverlap3D(
	Vector2 const&    cylinder1CenterXY,
	float             cylinder1Radius,
	FloatRange const& cylinder1MinMaxZ,
	Vector2 const&    cylinder2CenterXY,
	float             cylinder2Radius,
	FloatRange const& cylinder2MinMaxZ);
bool DoCylinderZsOverlap3D(
	Vector3 const& centerA, float radiusA, float heightA, Vector3 const& centerB, float radiusB, float heightB);

bool DoSphereAndAABBOverlap3D(
	Vector3 const& sphereCenter, float sphereRadius, Vector3 const& boxMins, Vector3 const& boxMaxs);
bool DoSphereAndAABBOverlap3D(Vector3 const& sphereCenter, float sphereRadius, AABB3 const& box);
bool DoSphereAndAABBOverlap3D(Sphere3 const& sphere, AABB3 const& box);

bool DoCylinderZAndAABBOverlap3D(
	Vector2 const&    cylinderCenterXY,
	float             cylinderRadius,
	FloatRange const& cylinderMinMaxZ,
	Vector3 const&    boxMins,
	Vector3 const&    boxMaxs);
bool DoCylinderZAndAABBOverlap3D(
	Vector2 const& cylinderCenterXY, float cylinderRadius, FloatRange const& cylinderMinMaxZ, AABB3 const& box);
bool DoCylinderZAndAABBOverlap3D(CylinderZ3 const& cylinder, AABB3 const& box);

bool DoCylinderZAndSphereOverlap3D(
	Vector2 const&    cylinderCenterXY,
	float             cylinderRadius,
	FloatRange const& cylinderMinMaxZ,
	Vector3 const&    sphereCenter,
	float             sphereRadius);
bool DoCylinderZAndSphereOverlap3D(CylinderZ3 const& cylinder, Sphere3 const& sphere);

bool DoCapsulesOverlap3D(Capsule3 const& capsuleA, Capsule3 const& capsuleB);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Transforms (in-place helpers)
//------------------------------------------------------------------------------------------------
#pragma region Transforms

void TransformPosition2D(Vector2& pos, float scale, float rotationDegrees, Vector2 const& translation);
void TransformPosition2D(Vector2& pos, Vector2 const& iBasis, Vector2 const& jBasis, Vector2 const& translation);
void TransformPositionXY3D(Vector3& pos, float scaleXY, float zRotationDegrees, Vector2 const& translationXY);
void TransformPositionXY3D(
	Vector3& pos, Vector2 const& iBasisXY, Vector2 const& jBasisXY, Vector2 const& translationXY);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Interpolation, Mapping, Clamping
//------------------------------------------------------------------------------------------------
#pragma region Interpolation / Mapping / Clamping

float       Interpolate(float start, float end, float fraction);
Vector3     Interpolate(Vector3 const& start, Vector3 const& end, float fraction);
Color       Interpolate(Color const& start, Color const& end, float fraction);
EulerAngles Interpolate(EulerAngles const& start, EulerAngles const& end, float fraction);

float       InterpolateClamped(float start, float end, float fraction);
Vector3     InterpolateClamped(Vector3 const& start, Vector3 const& end, float fraction);
Color       InterpolateClamped(Color const& start, Color const& end, float fraction);
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

Vector2 ComputeCubicBezier2D(Vector2 const& A, Vector2 const& B, Vector2 const& C, Vector2 const& D, float t);
Vector2 ComputeQuinticBezier2D(
	Vector2 const& A,
	Vector2 const& B,
	Vector2 const& C,
	Vector2 const& D,
	Vector2 const& E,
	Vector2 const& F,
	float          t);

Vector3 ComputeCubicBezier3D(Vector3 const& A, Vector3 const& B, Vector3 const& C, Vector3 const& D, float t);
Vector3 ComputeQuinticBezier3D(
	Vector3 const& A,
	Vector3 const& B,
	Vector3 const& C,
	Vector3 const& D,
	Vector3 const& E,
	Vector3 const& F,
	float          t);

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

float DotProduct2D(Vector2 const& a, Vector2 const& b);
float DotProduct3D(Vector3 const& a, Vector3 const& b);
float DotProduct4D(Vector4 const& a, Vector4 const& b);

// About the cross product direction
// 1) Show your right hand!
// 2) Point at the first vector with your index finger
// 3) Point at the second vector with your middle finger
// 4) Your thumb will now be pointing in the direction of the cross product result
float   CrossProduct2D(Vector2 const& a, Vector2 const& b);
Vector3 CrossProduct3D(Vector3 const& a, Vector3 const& b);

float   GetProjectedLength2D(Vector2 const& vector, Vector2 const& basis);
Vector2 GetProjectedVector2D(Vector2 const& vector, Vector2 const& basis);
Vector3 GetProjectedVector3D(Vector3 const& vector, Vector3 const& basis);
float   GetAngleDegreesBetweenVectors2D(Vector2 const& a, Vector2 const& b);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry: Push Out (collision resolution helpers)
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Push Out

bool PushDiscOutOfFixedPoint2D(Vector2& discCenter, float discRadius, Vector2 const& fixedPoint);
bool PushDiscOutOfFixedDisc2D(
	Vector2& discCenter, float discRadius, Vector2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscOutOfFixedDisc2D(Disc2& discToPush, Disc2 const& fixedDisc);
bool PushDiscsOutOfEachOther2D(Vector2& discCenterA, float discRadiusA, Vector2& discCenterB, float discRadiusB);
bool PushDiscsOutOfEachOther2D(Disc2& discA, Disc2& discB);
bool PushDiscOutOfFixedAABB2D(Vector2& discCenter, float discRadius, AABB2 const& box);
bool PushDiscOutOfFixedAABB2D(Disc2& discToPush, AABB2 const& box);
bool PushDiscOutOfFixedInfiniteLine2D(
	Vector2& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd);
bool PushDiscOutOfFixedInfiniteLine2D(Disc2& discToPush, LineSegment2 const& line);
bool PushDiscOutOfFixedLine2D(Vector2& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd);
bool PushDiscOutOfFixedLine2D(Disc2& discToPush, LineSegment2 const& line);
bool PushDiscOutOfFixedCapsule2D(
	Vector2& discCenter, float discRadius, Vector2 const& capsuleStart, Vector2 const& capsuleEnd, float capsuleRadius);
bool PushDiscOutOfFixedCapsule2D(Disc2& discToPush, Capsule2 const& capsule);
bool PushDiscOutOfFixedOBB2D(Vector2& discCenter, float discRadius, OBB2 const& box);
bool PushDiscOutOfFixedOBB2D(Disc2& discToPush, OBB2 const& box);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry Queries: Point Inside Tests
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Point Inside

bool IsPointInsideDisc2D(Vector2 point, Vector2 discCenter, float discRadius);
bool IsPointInsideDisc2D(Vector2 point, Disc2 const& disc);
bool IsPointInsideAABB2D(Vector2 point, AABB2 const& alignedBox);
bool IsPointInsideAABB3D(Vector3 point, Vector3 const& boxMins, Vector3 const& boxMaxs);
bool IsPointInsideAABB3D(Vector3 point, AABB3 const& alignedBox);
bool IsPointInsideOBB2D(Vector2 point, OBB2 const& orientedBox);
bool IsPointInsideCapsule2D(Vector2 point, Vector2 boneStart, Vector2 boneEnd, float radius);
bool IsPointInsideCapsule2D(Vector2 point, Capsule2 const& capsule);
bool IsPointInsideTriangle2D(
	Vector2 point, Vector2 ccw0, Vector2 ccw1, Vector2 ccw2); // Counter-Clockwise (positive winding)
bool IsPointInsideTriangle2D(Vector2 point, Triangle2 const& triangle);
bool IsPointInsideOrientedSector2D(
	Vector2 point, Vector2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(
	Vector2 point, Vector2 sectorOrigin, Vector2 sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Geometry Queries: Nearest Point
//------------------------------------------------------------------------------------------------
#pragma region Geometry - Nearest Point

Vector2 GetNearestPointOnDisc2D(Vector2 referencePos, Vector2 discCenter, float discRadius);
Vector2 GetNearestPointOnDisc2D(Vector2 referencePos, Disc2 const& disc);
Vector2 GetNearestPointOnAABB2D(Vector2 referencePos, AABB2 const& alignedBox);
Vector2 GetNearestPointOnOBB2D(Vector2 referencePos, OBB2 const& orientedBox);
Vector2 GetNearestPointOnInfiniteLine2D(Vector2 referencePos, Vector2 pointOnLine, Vector2 anotherPointOnLine);
Vector2 GetNearestPointOnInfiniteLine2D(Vector2 referencePos, LineSegment2 const& lineSegmentOnInfiniteLine);
Vector2 GetNearestPointOnLineSegment2D(Vector2 referencePos, Vector2 start, Vector2 end);
Vector2 GetNearestPointOnLineSegment2D(Vector2 referencePos, LineSegment2 const& lineSegment);
Vector2 GetNearestPointOnCapsule2D(Vector2 referencePos, Vector2 boneStart, Vector2 boneEnd, float radius);
Vector2 GetNearestPointOnCapsule2D(Vector2 referencePos, Capsule2 const& capsule);
Vector2 GetNearestPointOnTriangle2D(
	Vector2 referencePos, Vector2 ccw0, Vector2 ccw1, Vector2 ccw2); // Counter-Clockwise (positive winding)
Vector2 GetNearestPointOnTriangle2D(Vector2 referencePos, Triangle2 const& triangle);
Vector3 GetNearestPointOnAABB3D(Vector3 referencePos, AABB3 const& alignedBox);
Vector3 GetNearestPointOnAABB3D(Vector3 referencePos, Vector3 const& boxMins, Vector3 const& boxMaxs);
Vector3 GetNearestPointOnZCylinder3D(Vector3 referencePos, CylinderZ3 const& cylinder);
Vector3 GetNearestPointOnZCylinder3D(
	Vector3 referencePos, Vector3 const& cylinderStart, float cylinderHeight, float cylinderRadius);
Vector3 GetNearestPointOnSphere3D(Vector3 referencePos, Sphere3 const& sphere);
Vector3 GetNearestPointOnSphere3D(Vector3 referencePos, Vector3 const& sphereCenter, float sphereRadius);

Vector3 GetNearestPointOnLine3D(Vector3 referencePos, Vector3 const& lineStart, Vector3 const& lineEnd);
Vector3 GetNearestPointOnCapsule3D(Vector3 referencePos, Capsule3 const& capsule);
Vector3 GetNearestPointOnTriangle3D(Vector3 referencePos, Vector3 const& v0, Vector3 const& v1, Vector3 const& v2);
Vector3 GetNearestPointOnTriangle3D(Vector3 referencePos, Triangle3 const& triangle);

#pragma endregion

//------------------------------------------------------------------------------------------------
// Misc / Integer grid helpers
//------------------------------------------------------------------------------------------------
#pragma region Misc

int GetTaxicabDistance2D(IntVec2 const& a, IntVec2 const& b);
int GetTaxicabDistance2D(Vector2 const& a, Vector2 const& b);

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
	const Vector3&   billboardPosition = Vector3::Zero,
	const Vector2&   billboardScale    = Vector2::One);

#pragma endregion

} // namespace Math
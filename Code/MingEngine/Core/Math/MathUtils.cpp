#include "MingEngine/Core/Math/MathUtils.hpp"

#include <math.h>

namespace Math
{

float Abs(float value) { return (value < 0.f) ? -value : value; }

int Abs(int value) { return (value < 0) ? -value : value; }

float Sign(float value)
{
	if (value > 0.f)
	{
		return 1.f;
	}
	else if (value < 0.f)
	{
		return -1.f;
	}
	else
	{
		return 0.f;
	}
}

float Min(float a, float b) { return (a < b) ? a : b; }

int Min(int a, int b) { return (a < b) ? a : b; }

float Max(float a, float b) { return (a > b) ? a : b; }

int Max(int a, int b) { return (a > b) ? a : b; }

float ConvertDegreesToRadians(float degrees) { return degrees * kDegreesToRadiansMultiplier; }

float ConvertRadiansToDegrees(float radians) { return radians * kRadiansToDegreesMultiplier; }

float CosDegrees(float degrees) { return cosf(ConvertDegreesToRadians(degrees)); }

float SinDegrees(float degrees) { return sinf(ConvertDegreesToRadians(degrees)); }

float Atan2Degrees(float y, float x) { return ConvertRadiansToDegrees(atan2f(y, x)); }

float GetDistance2D(Vector2 const& a, Vector2 const& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return sqrtf(dx * dx + dy * dy);
}

float GetDistanceSquared2D(Vector2 const& a, Vector2 const& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return dx * dx + dy * dy;
}

float GetDistance3D(Vector3 const& a, Vector3 const& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

float GetDistanceXY3D(Vector3 const& a, Vector3 const& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return sqrtf(dx * dx + dy * dy);
}

float GetDistanceSquared3D(Vector3 const& a, Vector3 const& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return dx * dx + dy * dy + dz * dz;
}

float GetDistanceXYSquared3D(Vector3 const& a, Vector3 const& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return dx * dx + dy * dy;
}

bool DoDiscsOverlap2D(Vector2 const& centerA, float radiusA, Vector2 const& centerB, float radiusB)
{
	float distSquared = GetDistanceSquared2D(centerA, centerB);
	float radiiSum    = radiusA + radiusB;
	return distSquared <= (radiiSum * radiiSum);
}

bool DoDiscsOverlap2D(Disc2 const& discA, Disc2 const& discB)
{
	return DoDiscsOverlap2D(discA.m_center, discA.m_radius, discB.m_center, discB.m_radius);
}

bool DoDiscAndInfiniteLineOverlap2D(
	Vector2 const& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd)
{
	Vector2 nearestPoint = GetNearestPointOnInfiniteLine2D(discCenter, lineStart, lineEnd);
	float   distSquared  = GetDistanceSquared2D(discCenter, nearestPoint);
	return distSquared <= (discRadius * discRadius);
}

bool DoDiscAndInfiniteLineOverlap2D(Disc2 const& disc, LineSegment2 const& line)
{
	return DoDiscAndInfiniteLineOverlap2D(disc.m_center, disc.m_radius, line.m_start, line.m_end);
}

bool DoDiscAndLineOverlap2D(
	Vector2 const& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd)
{
	Vector2 nearestPoint = GetNearestPointOnLineSegment2D(discCenter, lineStart, lineEnd);
	float   distSquared  = GetDistanceSquared2D(discCenter, nearestPoint);
	return distSquared <= (discRadius * discRadius);
}

bool DoDiscAndLineOverlap2D(Disc2 const& disc, LineSegment2 const& line)
{
	return DoDiscAndLineOverlap2D(disc.m_center, disc.m_radius, line.m_start, line.m_end);
}

bool DoDiscAndAABBOverlap2D(Vector2 const& discCenter, float discRadius, Vector2 const& boxMins, Vector2 const& boxMaxs)
{
	float nearestX      = GetClamped(discCenter.x, boxMins.x, boxMaxs.x);
	float nearestY      = GetClamped(discCenter.y, boxMins.y, boxMaxs.y);
	float dx            = discCenter.x - nearestX;
	float dy            = discCenter.y - nearestY;
	float distSquared   = dx * dx + dy * dy;
	float radiusSquared = discRadius * discRadius;
	return distSquared < radiusSquared;
}

bool DoDiscAndAABBOverlap2D(Vector2 const& discCenter, float discRadius, AABB2 const& box)
{
	return DoDiscAndAABBOverlap2D(discCenter, discRadius, box.m_mins, box.m_maxs);
}

bool DoDiscAndAABBOverlap2D(Disc2 const& disc, AABB2 const& box)
{
	return DoDiscAndAABBOverlap2D(disc.m_center, disc.m_radius, box);
}

bool DoDiscAndCapsuleOverlap2D(
	Vector2 const& discCenter,
	float          discRadius,
	Vector2 const& capsuleStart,
	Vector2 const& capsuleEnd,
	float          capsuleRadius)
{
	Vector2 nearestPoint = GetNearestPointOnLineSegment2D(discCenter, capsuleStart, capsuleEnd);
	float   distSquared  = GetDistanceSquared2D(discCenter, nearestPoint);
	float   radiiSum     = discRadius + capsuleRadius;
	return distSquared <= (radiiSum * radiiSum);
}

bool DoDiscAndCapsuleOverlap2D(Disc2 const& disc, Capsule2 const& capsule)
{
	return DoDiscAndCapsuleOverlap2D(
		disc.m_center,
		disc.m_radius,
		capsule.m_bone.m_start,
		capsule.m_bone.m_end,
		capsule.m_radius);
}

bool DoDiscAndOBBOverlap2D(Vector2 const& discCenter, float discRadius, OBB2 const& box)
{
	Vector2 nearestPoint = GetNearestPointOnOBB2D(discCenter, box);
	float   distSquared  = GetDistanceSquared2D(discCenter, nearestPoint);
	return distSquared <= (discRadius * discRadius);
}

bool DoDiscAndOBBOverlap2D(Disc2 const& disc, OBB2 const& box)
{
	return DoDiscAndOBBOverlap2D(disc.m_center, disc.m_radius, box);
}

bool DoAABB3sOverlap3D(
	Vector3 const& firstMins, Vector3 const& firstMaxs, Vector3 const& secondMins, Vector3 const& secondMaxs)
{
	bool overlapsX = (firstMins.x < secondMaxs.x) && (secondMins.x < firstMaxs.x);
	bool overlapsY = (firstMins.y < secondMaxs.y) && (secondMins.y < firstMaxs.y);
	bool overlapsZ = (firstMins.z < secondMaxs.z) && (secondMins.z < firstMaxs.z);
	return overlapsX && overlapsY && overlapsZ;
}

bool DoAABB3sOverlap3D(AABB3 const& first, AABB3 const& second)
{
	return DoAABB3sOverlap3D(first.m_mins, first.m_maxs, second.m_mins, second.m_maxs);
}

bool DoSpheresOverlap3D(Vector3 const& centerA, float radiusA, Vector3 const& centerB, float radiusB)
{
	float distSquared = GetDistanceSquared3D(centerA, centerB);
	float radiiSum    = radiusA + radiusB;
	return distSquared < (radiiSum * radiiSum);
}

bool DoSpheresOverlap3D(Sphere3 const& a, Sphere3 const& b)
{
	return DoSpheresOverlap3D(a.m_center, a.m_radius, b.m_center, b.m_radius);
}

bool DoCylinderZsOverlap3D(
	Vector2 const&    cylinder1CenterXY,
	float             cylinder1Radius,
	FloatRange const& cylinder1MinMaxZ,
	Vector2 const&    cylinder2CenterXY,
	float             cylinder2Radius,
	FloatRange const& cylinder2MinMaxZ)
{
	float distSquaredXY = GetDistanceSquared2D(cylinder1CenterXY, cylinder2CenterXY);
	float radiiSum      = cylinder1Radius + cylinder2Radius;
	bool  overlapInXY   = distSquaredXY < (radiiSum * radiiSum);

	bool overlapInZ = cylinder1MinMaxZ.IsOverlap(cylinder2MinMaxZ);
	return overlapInXY && overlapInZ;
}

bool DoCylinderZsOverlap3D(
	Vector3 const& centerA, float radiusA, float heightA, Vector3 const& centerB, float radiusB, float heightB)
{
	FloatRange rangeA(centerA.z - (heightA * 0.5f), centerA.z + (heightA * 0.5f));
	FloatRange rangeB(centerB.z - (heightB * 0.5f), centerB.z + (heightB * 0.5f));
	return DoCylinderZsOverlap3D(
		Vector2(centerA.x, centerA.y),
		radiusA,
		rangeA,
		Vector2(centerB.x, centerB.y),
		radiusB,
		rangeB);
}

bool DoSphereAndAABBOverlap3D(
	Vector3 const& sphereCenter, float sphereRadius, Vector3 const& boxMins, Vector3 const& boxMaxs)
{
	Vector3 nearestPoint    = GetNearestPointOnAABB3D(sphereCenter, boxMins, boxMaxs);
	float   distanceSquared = GetDistanceSquared3D(sphereCenter, nearestPoint);
	return distanceSquared < (sphereRadius * sphereRadius);
}

bool DoSphereAndAABBOverlap3D(Vector3 const& sphereCenter, float sphereRadius, AABB3 const& box)
{
	return DoSphereAndAABBOverlap3D(sphereCenter, sphereRadius, box.m_mins, box.m_maxs);
}

bool DoSphereAndAABBOverlap3D(Sphere3 const& sphere, AABB3 const& box)
{
	return DoSphereAndAABBOverlap3D(sphere.m_center, sphere.m_radius, box);
}

bool DoCylinderZAndAABBOverlap3D(
	Vector2 const&    cylinderCenterXY,
	float             cylinderRadius,
	FloatRange const& cylinderMinMaxZ,
	Vector3 const&    boxMins,
	Vector3 const&    boxMaxs)
{
	float nearestX    = GetClamped(cylinderCenterXY.x, boxMins.x, boxMaxs.x);
	float nearestY    = GetClamped(cylinderCenterXY.y, boxMins.y, boxMaxs.y);
	float dx          = cylinderCenterXY.x - nearestX;
	float dy          = cylinderCenterXY.y - nearestY;
	bool  overlapInXY = (dx * dx + dy * dy) < (cylinderRadius * cylinderRadius);

	FloatRange boxRangeZ(boxMins.z, boxMaxs.z);
	bool       overlapInZ = cylinderMinMaxZ.IsOverlap(boxRangeZ);
	return overlapInXY && overlapInZ;
}

bool DoCylinderZAndAABBOverlap3D(
	Vector2 const& cylinderCenterXY, float cylinderRadius, FloatRange const& cylinderMinMaxZ, AABB3 const& box)
{
	return DoCylinderZAndAABBOverlap3D(cylinderCenterXY, cylinderRadius, cylinderMinMaxZ, box.m_mins, box.m_maxs);
}

bool DoCylinderZAndAABBOverlap3D(CylinderZ3 const& cylinder, AABB3 const& box)
{
	return DoCylinderZAndAABBOverlap3D(cylinder.m_centerXY, cylinder.m_radius, cylinder.m_minMaxZ, box);
}

bool DoCylinderZAndSphereOverlap3D(
	Vector2 const&    cylinderCenterXY,
	float             cylinderRadius,
	FloatRange const& cylinderMinMaxZ,
	Vector3 const&    sphereCenter,
	float             sphereRadius)
{
	float   cylinderMinZ = cylinderMinMaxZ.m_min;
	float   cylinderMaxZ = cylinderMinMaxZ.m_max;
	Vector3 cylinderStart(cylinderCenterXY.x, cylinderCenterXY.y, cylinderMinZ);
	float   cylinderHeight = cylinderMaxZ - cylinderMinZ;

	Vector3 nearestPoint    = GetNearestPointOnZCylinder3D(sphereCenter, cylinderStart, cylinderHeight, cylinderRadius);
	float   distanceSquared = GetDistanceSquared3D(sphereCenter, nearestPoint);
	return distanceSquared < (sphereRadius * sphereRadius);
}

bool DoCylinderZAndSphereOverlap3D(CylinderZ3 const& cylinder, Sphere3 const& sphere)
{
	return DoCylinderZAndSphereOverlap3D(
		cylinder.m_centerXY,
		cylinder.m_radius,
		cylinder.m_minMaxZ,
		sphere.m_center,
		sphere.m_radius);
}

// Capsule overlap = closest distance between their center segments <= radius sum.
// 1) Represent center segments as:
//      P(t1) = A0 + t1 * d1,  Q(t2) = B0 + t2 * d2,  t1,t2 in [0,1]
// 2) Minimize squared distance:
//      F(t1,t2) = |P(t1) - Q(t2)|^2 = |r + t1*d1 - t2*d2|^2
// 3) Set partial derivatives to zero and solve the 2x2 system:
//      t1 = (b*f - c*e) / (a*e - b*b)
//      t2 = (a*f - c*b) / (a*e - b*b)
// 4) If t1,t2 are inside [0,1], use them; otherwise test endpoint projections.
// 5) Capsules overlap if closestDistSq < (radiusA + radiusB)^2.
bool DoCapsulesOverlap3D(Capsule3 const& capsuleA, Capsule3 const& capsuleB)
{
	Vector3 d1 = capsuleA.m_end - capsuleA.m_start;
	Vector3 d2 = capsuleB.m_end - capsuleB.m_start;
	Vector3 r  = capsuleA.m_start - capsuleB.m_start;

	float a = DotProduct3D(d1, d1); // d1·d1
	float e = DotProduct3D(d2, d2); // d2·d2
	float b = DotProduct3D(d1, d2); // d1·d2
	float c = DotProduct3D(d1, r);  // d1·r
	float f = DotProduct3D(d2, r);  // d2·r

	float denom = a * e - b * b;
	float t1    = (b * f - c * e) / denom;
	float t2    = (a * f - c * b) / denom;

	Vector3 closestPointA = capsuleA.m_start + t1 * d1;
	Vector3 closestPointB = capsuleB.m_start + t2 * d2;

	if (t1 >= 0.f && t1 <= 1.f && t2 >= 0.f && t2 <= 1.f)
	{
		closestPointA = capsuleA.m_start + d1 * t1;
		closestPointB = capsuleB.m_start + d2 * t2;
	}
	else
	{
		float bestDistSq         = 1e9f;
		auto  UpdateClosestPoint = [&](Vector3 const& pointA, Vector3 const& pointB)
		{
			float distSq = (pointA - pointB).GetLengthSquared();

			if (distSq < bestDistSq)
			{
				bestDistSq    = distSq;
				closestPointA = pointA;
				closestPointB = pointB;
			}
		};

		UpdateClosestPoint(
			capsuleA.m_start,
			GetNearestPointOnLine3D(capsuleA.m_start, capsuleB.m_start, capsuleB.m_end));
		UpdateClosestPoint(capsuleA.m_end, GetNearestPointOnLine3D(capsuleA.m_end, capsuleB.m_start, capsuleB.m_end));
		UpdateClosestPoint(
			capsuleB.m_start,
			GetNearestPointOnLine3D(capsuleB.m_start, capsuleA.m_start, capsuleA.m_end));
		UpdateClosestPoint(capsuleB.m_end, GetNearestPointOnLine3D(capsuleB.m_end, capsuleA.m_start, capsuleA.m_end));
	}

	float radiusSumSq = (capsuleA.m_radius + capsuleB.m_radius) * (capsuleA.m_radius + capsuleB.m_radius);
	float distSq      = GetDistanceSquared3D(closestPointA, closestPointB);

	return distSq < radiusSumSq;
}

void TransformPosition2D(Vector2& pos, float scale, float rotationDegrees, Vector2 const& translation)
{
	pos *= scale;
	pos.RotateDegrees(rotationDegrees);
	pos += translation;
}

void TransformPosition2D(Vector2& pos, Vector2 const& iBasis, Vector2 const& jBasis, Vector2 const& translation)
{
	float x = pos.x;
	float y = pos.y;
	pos     = iBasis * x + jBasis * y + translation;
}

void TransformPositionXY3D(Vector3& pos, float scaleXY, float zRotationDegrees, Vector2 const& translationXY)
{
	pos.x *= scaleXY;
	pos.y *= scaleXY;
	pos = pos.GetRotatedAboutZDegrees(zRotationDegrees);
	pos.x += translationXY.x;
	pos.y += translationXY.y;
}

void TransformPositionXY3D(Vector3& pos, Vector2 const& iBasisXY, Vector2 const& jBasisXY, Vector2 const& translationXY)
{
	float   x     = pos.x;
	float   y     = pos.y;
	Vector2 pos2D = iBasisXY * x + jBasisXY * y + translationXY;
	pos.x         = pos2D.x;
	pos.y         = pos2D.y;
}

float Interpolate(float start, float end, float fraction) { return start * (1.0f - fraction) + end * fraction; }

Vector3 Interpolate(Vector3 const& start, Vector3 const& end, float fraction)
{
	return Vector3::Interpolate(start, end, fraction);
}

Color Interpolate(Color const& start, Color const& end, float fraction)
{
	float r = Interpolate(::NormalizeByte(start.r), ::NormalizeByte(end.r), fraction);
	float g = Interpolate(::NormalizeByte(start.g), ::NormalizeByte(end.g), fraction);
	float b = Interpolate(::NormalizeByte(start.b), ::NormalizeByte(end.b), fraction);
	float a = Interpolate(::NormalizeByte(start.a), ::NormalizeByte(end.a), fraction);

	return Color(::DenormalizeByte(r), ::DenormalizeByte(g), ::DenormalizeByte(b), ::DenormalizeByte(a));
}

EulerAngles Interpolate(EulerAngles const& start, EulerAngles const& end, float fraction)
{
	return EulerAngles::Interpolate(start, end, fraction);
}

float SmoothStart2(float t) { return t * t; }

float SmoothStart3(float t)
{
	float const t2 = t * t;
	return t2 * t;
}

float SmoothStart4(float t)
{
	float const t2 = t * t;
	return t2 * t2;
}

float SmoothStart5(float t)
{
	float const t2 = t * t;
	return t2 * t2 * t;
}

float SmoothStart6(float t)
{
	float const t2 = t * t;
	return t2 * t2 * t2;
}

float SmoothStop2(float t)
{
	float const oneMinusT = 1.f - t;
	return 1.f - oneMinusT * oneMinusT;
}

float SmoothStop3(float t)
{
	float const oneMinusT = 1.f - t;
	float const omt2      = oneMinusT * oneMinusT;
	return 1.f - omt2 * oneMinusT;
}

float SmoothStop4(float t)
{
	float const oneMinusT = 1.f - t;
	float const omt2      = oneMinusT * oneMinusT;
	return 1.f - omt2 * omt2;
}

float SmoothStop5(float t)
{
	float const oneMinusT = 1.f - t;
	float const omt2      = oneMinusT * oneMinusT;
	return 1.f - omt2 * omt2 * oneMinusT;
}

float SmoothStop6(float t)
{
	float const oneMinusT = 1.f - t;
	float const omt2      = oneMinusT * oneMinusT;
	return 1.f - omt2 * omt2 * omt2;
}

float SmoothStep3(float t) { return -2 * t * t * t + 3 * t * t; }

float SmoothStep5(float t)
{
	float const t2 = t * t;
	float const t3 = t2 * t;
	return t3 * (t * (t * 6.f - 15.f) + 10.f);
}

float Hesitate3(float t)
{
	float const t2 = t * t;
	float const t3 = t2 * t;
	return 3.f * t - 6.f * t2 + 4.f * t3;
}

float Hesitate5(float t)
{
	float const t2 = t * t;
	float const t3 = t2 * t;
	float const t4 = t3 * t;
	float const t5 = t4 * t;
	return 5.f * t - 20.f * t2 + 40.f * t3 - 40.f * t4 + 16.f * t5;
}

float String(float t)
{
	auto fastExpDecay = [](float x) { return 1.0f / (1.0f + x + 0.5f * x * x); };

	float frequency = 4.0f;
	float decay     = 8.0f;
	float d         = fastExpDecay(decay * t);

	// When cos == 1, tangent == 0
	return 1.0f - d * CosDegrees(frequency * t * 360.f);
}

float String(float t, float frequency, float decay)
{
	auto fastExpDecay = [](float x) { return 1.0f / (1.0f + x + 0.5f * x * x); };

	float d = fastExpDecay(decay * t);

	// When cos == 1, tangent == 0
	return 1.0f - d * CosDegrees(frequency * t * 360.f);
}

float ComputeCubicBezier1D(float A, float B, float C, float D, float t)
{
	float const ab = Interpolate(A, B, t);
	float const bc = Interpolate(B, C, t);
	float const cd = Interpolate(C, D, t);

	float const abc = Interpolate(ab, bc, t);
	float const bcd = Interpolate(bc, cd, t);

	return Interpolate(abc, bcd, t);
}

float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t)
{
	float const ab = Interpolate(A, B, t);
	float const bc = Interpolate(B, C, t);
	float const cd = Interpolate(C, D, t);
	float const de = Interpolate(D, E, t);
	float const ef = Interpolate(E, F, t);

	float const abc = Interpolate(ab, bc, t);
	float const bcd = Interpolate(bc, cd, t);
	float const cde = Interpolate(cd, de, t);
	float const def = Interpolate(de, ef, t);

	float const abcd = Interpolate(abc, bcd, t);
	float const bcde = Interpolate(bcd, cde, t);
	float const cdef = Interpolate(cde, def, t);

	float const abcde = Interpolate(abcd, bcde, t);
	float const bcdef = Interpolate(bcde, cdef, t);

	return Interpolate(abcde, bcdef, t);
}

Vector2 ComputeCubicBezier2D(Vector2 const& A, Vector2 const& B, Vector2 const& C, Vector2 const& D, float t)
{
	Vector2 const ab = Vector2(Interpolate(A.x, B.x, t), Interpolate(A.y, B.y, t));
	Vector2 const bc = Vector2(Interpolate(B.x, C.x, t), Interpolate(B.y, C.y, t));
	Vector2 const cd = Vector2(Interpolate(C.x, D.x, t), Interpolate(C.y, D.y, t));

	Vector2 const abc = Vector2(Interpolate(ab.x, bc.x, t), Interpolate(ab.y, bc.y, t));
	Vector2 const bcd = Vector2(Interpolate(bc.x, cd.x, t), Interpolate(bc.y, cd.y, t));

	return Vector2(Interpolate(abc.x, bcd.x, t), Interpolate(abc.y, bcd.y, t));
}

Vector2 ComputeQuinticBezier2D(
	Vector2 const& A, Vector2 const& B, Vector2 const& C, Vector2 const& D, Vector2 const& E, Vector2 const& F, float t)
{
	Vector2 const ab = Vector2(Interpolate(A.x, B.x, t), Interpolate(A.y, B.y, t));
	Vector2 const bc = Vector2(Interpolate(B.x, C.x, t), Interpolate(B.y, C.y, t));
	Vector2 const cd = Vector2(Interpolate(C.x, D.x, t), Interpolate(C.y, D.y, t));
	Vector2 const de = Vector2(Interpolate(D.x, E.x, t), Interpolate(D.y, E.y, t));
	Vector2 const ef = Vector2(Interpolate(E.x, F.x, t), Interpolate(E.y, F.y, t));

	Vector2 const abc = Vector2(Interpolate(ab.x, bc.x, t), Interpolate(ab.y, bc.y, t));
	Vector2 const bcd = Vector2(Interpolate(bc.x, cd.x, t), Interpolate(bc.y, cd.y, t));
	Vector2 const cde = Vector2(Interpolate(cd.x, de.x, t), Interpolate(cd.y, de.y, t));
	Vector2 const def = Vector2(Interpolate(de.x, ef.x, t), Interpolate(de.y, ef.y, t));

	Vector2 const abcd = Vector2(Interpolate(abc.x, bcd.x, t), Interpolate(abc.y, bcd.y, t));
	Vector2 const bcde = Vector2(Interpolate(bcd.x, cde.x, t), Interpolate(bcd.y, cde.y, t));
	Vector2 const cdef = Vector2(Interpolate(cde.x, def.x, t), Interpolate(cde.y, def.y, t));

	Vector2 const abcde = Vector2(Interpolate(abcd.x, bcde.x, t), Interpolate(abcd.y, bcde.y, t));
	Vector2 const bcdef = Vector2(Interpolate(bcde.x, cdef.x, t), Interpolate(bcde.y, cdef.y, t));

	return Vector2(Interpolate(abcde.x, bcdef.x, t), Interpolate(abcde.y, bcdef.y, t));
}

Vector3 ComputeCubicBezier3D(Vector3 const& A, Vector3 const& B, Vector3 const& C, Vector3 const& D, float t)
{
	Vector3 const ab = Interpolate(A, B, t);
	Vector3 const bc = Interpolate(B, C, t);
	Vector3 const cd = Interpolate(C, D, t);

	Vector3 const abc = Interpolate(ab, bc, t);
	Vector3 const bcd = Interpolate(bc, cd, t);

	return Interpolate(abc, bcd, t);
}

Vector3 ComputeQuinticBezier3D(
	Vector3 const& A, Vector3 const& B, Vector3 const& C, Vector3 const& D, Vector3 const& E, Vector3 const& F, float t)
{
	Vector3 const ab = Interpolate(A, B, t);
	Vector3 const bc = Interpolate(B, C, t);
	Vector3 const cd = Interpolate(C, D, t);
	Vector3 const de = Interpolate(D, E, t);
	Vector3 const ef = Interpolate(E, F, t);

	Vector3 const abc = Interpolate(ab, bc, t);
	Vector3 const bcd = Interpolate(bc, cd, t);
	Vector3 const cde = Interpolate(cd, de, t);
	Vector3 const def = Interpolate(de, ef, t);

	Vector3 const abcd = Interpolate(abc, bcd, t);
	Vector3 const bcde = Interpolate(bcd, cde, t);
	Vector3 const cdef = Interpolate(cde, def, t);

	Vector3 const abcde = Interpolate(abcd, bcde, t);
	Vector3 const bcdef = Interpolate(bcde, cdef, t);

	return Interpolate(abcde, bcdef, t);
}

float InterpolateClamped(float start, float end, float fraction)
{
	float f = GetClampedZeroToOne(fraction);
	return Interpolate(start, end, f);
}

Vector3 InterpolateClamped(Vector3 const& start, Vector3 const& end, float fraction)
{
	return Vector3::InterpolateClamped(start, end, fraction);
}

Color InterpolateClamped(Color const& start, Color const& end, float fraction)
{
	float f = GetClampedZeroToOne(fraction);
	return Interpolate(start, end, f);
}

EulerAngles InterpolateClamped(EulerAngles const& start, EulerAngles const& end, float fraction)
{
	return EulerAngles::InterpolateClamped(start, end, fraction);
}

float GetFractionWithinRange(float value, float start, float end) { return (value - start) / (end - start); }

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
	return Interpolate(outStart, outEnd, fraction);
}

float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
	fraction       = GetClampedZeroToOne(fraction);
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

int GetClamped(int value, int minValue, int maxValue)
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

/// <summary>
/// Returns the shortest angular displacement from startDegrees to endDegrees in the range [-180, 180].
/// </summary>
/// <param name="startDegrees"></param>
/// <param name="endDegrees"></param>
/// <returns></returns>
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

float DotProduct2D(Vector2 const& a, Vector2 const& b) { return Vector2::DotProduct(a, b); }

float DotProduct3D(Vector3 const& a, Vector3 const& b) { return Vector3::DotProduct(a, b); }

float DotProduct4D(Vector4 const& a, Vector4 const& b) { return Vector4::DotProduct(a, b); }

float CrossProduct2D(Vector2 const& a, Vector2 const& b) { return Vector2::CrossProduct(a, b); }

Vector3 CrossProduct3D(Vector3 const& a, Vector3 const& b) { return Vector3::CrossProduct(a, b); }

Matrix4x4 GetBillboardTransform(
	BillboardType    billboardType,
	Matrix4x4 const& targetTransform,
	const Vector3&   billboardPosition,
	const Vector2&   billboardScale /*= Vec2(1.0f, 1.0f)*/
)
{
	Vector3 const billboardScale3D = Vector3(billboardScale.x, billboardScale.y, 1.0f);

	Matrix4x4 billboardTransform = Matrix4x4::MakeTranslation3D(billboardPosition);
	billboardTransform.AppendScaleNonUniform3D(billboardScale3D);

	switch (billboardType)
	{
	case BillboardType::NONE:
	{
		return billboardTransform;
	}

	case BillboardType::WORLD_UP_FACING:
	{
		Vector3 worldUp      = Vector3::Up;
		Vector3 toTarget     = targetTransform.GetTranslation3D() - billboardPosition;
		Vector3 toTargetOnXY = toTarget - GetProjectedVector3D(toTarget, worldUp);

		if (toTargetOnXY.GetLengthSquared() <= 1e-5f)
		{
			toTarget     = targetTransform.GetIBasis3D();
			toTargetOnXY = toTarget - GetProjectedVector3D(toTarget, worldUp);

			if (toTargetOnXY.GetLengthSquared() <= 1e-5f)
			{
				toTarget     = Vector3::Forward;
				toTargetOnXY = toTarget - GetProjectedVector3D(toTarget, worldUp);
			}
		}

		Vector3 iBasis = toTargetOnXY.GetNormalized();
		Vector3 jBasis = CrossProduct3D(worldUp, iBasis);
		jBasis.Normalize();

		billboardTransform = Matrix4x4(iBasis, jBasis, worldUp, billboardPosition);
		billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
		return billboardTransform;
	}

	case BillboardType::WORLD_UP_OPPOSING:
	{
		Vector3 worldUp        = Vector3::Up;
		Vector3 invTarget      = -targetTransform.GetIBasis3D();
		Vector3 fromTargetOnXY = invTarget - GetProjectedVector3D(invTarget, worldUp);

		if (fromTargetOnXY.GetLengthSquared() <= 1e-5f)
		{
			invTarget      = targetTransform.GetIBasis3D();
			fromTargetOnXY = invTarget - GetProjectedVector3D(invTarget, worldUp);

			if (fromTargetOnXY.GetLengthSquared() <= 1e-5f)
			{
				invTarget      = Vector3::Forward;
				fromTargetOnXY = invTarget - GetProjectedVector3D(invTarget, worldUp);
			}
		}

		Vector3 iBasis = fromTargetOnXY.GetNormalized();
		Vector3 jBasis = CrossProduct3D(worldUp, iBasis);
		jBasis.Normalize();

		billboardTransform = Matrix4x4(iBasis, jBasis, worldUp, billboardPosition);
		billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
		return billboardTransform;
	}

	case BillboardType::FULL_FACING:
	{
		Vector3 toTarget    = targetTransform.GetTranslation3D() - billboardPosition;
		Vector3 iBasis      = toTarget.GetNormalized();
		Vector3 referenceUp = targetTransform.GetKBasis3D();
		if (referenceUp.GetLengthSquared() <= 1e-5f)
		{
			referenceUp = Vector3::Up;
		}

		Vector3 jBasis = CrossProduct3D(referenceUp, iBasis);
		if (jBasis.GetLengthSquared() <= 1e-5f)
		{
			referenceUp = targetTransform.GetJBasis3D();
			jBasis      = CrossProduct3D(referenceUp, iBasis);

			if (jBasis.GetLengthSquared() <= 1e-5f)
			{
				referenceUp = Vector3::Up;
				jBasis      = CrossProduct3D(referenceUp, iBasis);
			}
		}

		jBasis.Normalize();
		Vector3 kBasis = CrossProduct3D(iBasis, jBasis);
		kBasis.Normalize();

		billboardTransform = Matrix4x4(iBasis, jBasis, kBasis, billboardPosition);
		billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
		return billboardTransform;
	}

	case BillboardType::FULL_OPPOSING:
	{
		Vector3 invTarget   = -targetTransform.GetIBasis3D();
		Vector3 iBasis      = invTarget.GetNormalized();
		Vector3 referenceUp = targetTransform.GetKBasis3D();
		if (referenceUp.GetLengthSquared() <= 1e-5f)
		{
			referenceUp = Vector3::Up;
		}

		Vector3 jBasis = CrossProduct3D(referenceUp, iBasis);
		if (jBasis.GetLengthSquared() <= 1e-5f)
		{
			referenceUp = targetTransform.GetJBasis3D();
			jBasis      = CrossProduct3D(referenceUp, iBasis);

			if (jBasis.GetLengthSquared() <= 1e-5f)
			{
				referenceUp = Vector3::Up;
				jBasis      = CrossProduct3D(referenceUp, iBasis);
			}
		}

		jBasis.Normalize();
		Vector3 kBasis = CrossProduct3D(iBasis, jBasis);
		kBasis.Normalize();

		billboardTransform = Matrix4x4(iBasis, jBasis, kBasis, billboardPosition);
		billboardTransform.AppendScaleNonUniform3D(billboardScale3D);
		return billboardTransform;
	}

	default:
		return billboardTransform;
	}
}

bool PushDiscOutOfFixedPoint2D(Vector2& discCenter, float discRadius, Vector2 const& fixedPoint)
{
	Vector2 toCenter = discCenter - fixedPoint;
	float   dist     = toCenter.GetLength();

	if (dist >= discRadius || dist == 0.f)
	{
		return false;
	}

	Vector2 pushDir = toCenter.GetNormalized();
	discCenter      = fixedPoint + pushDir * discRadius;
	return true;
}

bool PushDiscOutOfFixedDisc2D(
	Vector2& discCenter, float discRadius, Vector2 const& fixedDiscCenter, float fixedDiscRadius)
{
	Vector2 between = discCenter - fixedDiscCenter;
	float   dist    = between.GetLength();
	float   minDist = discRadius + fixedDiscRadius;

	if (dist >= minDist || dist == 0.f)
	{
		return false;
	}

	Vector2 pushDir = between.GetNormalized();
	discCenter      = fixedDiscCenter + pushDir * minDist;
	return true;
}

bool PushDiscOutOfFixedDisc2D(Disc2& discToPush, Disc2 const& fixedDisc)
{
	return PushDiscOutOfFixedDisc2D(discToPush.m_center, discToPush.m_radius, fixedDisc.m_center, fixedDisc.m_radius);
}

bool PushDiscsOutOfEachOther2D(Vector2& discCenterA, float discRadiusA, Vector2& discCenterB, float discRadiusB)
{
	Vector2 bToA     = discCenterA - discCenterB;
	float   bToADist = bToA.GetLength();
	float   minDist  = discRadiusA + discRadiusB;

	if (bToADist >= minDist || bToADist == 0.f)
	{
		return false;
	}

	Vector2 pushDir = bToA.GetNormalized();
	float   overlap = minDist - bToADist;
	discCenterA += pushDir * (overlap * 0.5f);
	discCenterB -= pushDir * (overlap * 0.5f);
	return true;
}

bool PushDiscsOutOfEachOther2D(Disc2& discA, Disc2& discB)
{
	return PushDiscsOutOfEachOther2D(discA.m_center, discA.m_radius, discB.m_center, discB.m_radius);
}

bool PushDiscOutOfFixedAABB2D(Vector2& discCenter, float discRadius, AABB2 const& box)
{
	float   minDist             = 1e9f;
	Vector2 minDir              = Vector2::Zero;
	auto    UpdateMinDistAndDir = [&](float dist, Vector2 const& dir)
	{
		if (dist < minDist)
		{
			minDist = dist;
			minDir  = dir;
		}
	};

	Vector2 nearest  = box.GetNearestPoint(discCenter);
	Vector2 toCenter = discCenter - nearest;
	float   dist     = toCenter.GetLength();

	if (!DoDiscAndAABBOverlap2D(discCenter, discRadius, box))
	{
		return false;
	}
	else if (dist == 0.f)
	{
		float leftDist  = Abs(nearest.x - box.m_mins.x);
		float rightDist = Abs(nearest.x - box.m_maxs.x);
		float upDist    = Abs(nearest.y - box.m_maxs.y);
		float downDist  = Abs(nearest.y - box.m_mins.y);

		UpdateMinDistAndDir(leftDist, Vector2(-1.f, 0.f));
		UpdateMinDistAndDir(rightDist, Vector2(1.f, 0.f));
		UpdateMinDistAndDir(upDist, Vector2(0.f, 1.f));
		UpdateMinDistAndDir(downDist, Vector2(0.f, -1.f));

		discCenter = minDir * (discRadius + minDist) + nearest;
		return true;
	}
	else
	{
		Vector2 pushDir = toCenter.GetNormalized();
		discCenter      = nearest + pushDir * discRadius;
		return true;
	}
}

bool PushDiscOutOfFixedAABB2D(Disc2& discToPush, AABB2 const& box)
{
	return PushDiscOutOfFixedAABB2D(discToPush.m_center, discToPush.m_radius, box);
}

bool PushDiscOutOfFixedInfiniteLine2D(
	Vector2& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd)
{
	Vector2 nearestPoint = GetNearestPointOnInfiniteLine2D(discCenter, lineStart, lineEnd);
	return PushDiscOutOfFixedPoint2D(discCenter, discRadius, nearestPoint);
}

bool PushDiscOutOfFixedInfiniteLine2D(Disc2& discToPush, LineSegment2 const& line)
{
	return PushDiscOutOfFixedInfiniteLine2D(discToPush.m_center, discToPush.m_radius, line.m_start, line.m_end);
}

bool PushDiscOutOfFixedLine2D(Vector2& discCenter, float discRadius, Vector2 const& lineStart, Vector2 const& lineEnd)
{
	Vector2 nearest = GetNearestPointOnLineSegment2D(discCenter, lineStart, lineEnd);
	return PushDiscOutOfFixedPoint2D(discCenter, discRadius, nearest);
}

bool PushDiscOutOfFixedLine2D(Disc2& discToPush, LineSegment2 const& line)
{
	return PushDiscOutOfFixedLine2D(discToPush.m_center, discToPush.m_radius, line.m_start, line.m_end);
}

bool PushDiscOutOfFixedCapsule2D(
	Vector2& discCenter, float discRadius, Vector2 const& capsuleStart, Vector2 const& capsuleEnd, float capsuleRadius)
{
	return PushDiscOutOfFixedLine2D(discCenter, discRadius + capsuleRadius, capsuleStart, capsuleEnd);
}

bool PushDiscOutOfFixedCapsule2D(Disc2& discToPush, Capsule2 const& capsule)
{
	return PushDiscOutOfFixedCapsule2D(
		discToPush.m_center,
		discToPush.m_radius,
		capsule.m_bone.m_start,
		capsule.m_bone.m_end,
		capsule.m_radius);
}

bool PushDiscOutOfFixedOBB2D(Vector2& discCenter, float discRadius, OBB2 const& box)
{
	Vector2 nearestPoint = GetNearestPointOnOBB2D(discCenter, box);
	return PushDiscOutOfFixedPoint2D(discCenter, discRadius, nearestPoint);
}

bool PushDiscOutOfFixedOBB2D(Disc2& discToPush, OBB2 const& box)
{
	return PushDiscOutOfFixedOBB2D(discToPush.m_center, discToPush.m_radius, box);
}

float GetProjectedLength2D(Vector2 const& vector, Vector2 const& basis)
{
	return Vector2::GetProjectedLength(vector, basis);
}

Vector2 GetProjectedVector2D(Vector2 const& vector, Vector2 const& basis)
{
	return Vector2::GetProjectedVector(vector, basis);
}

Vector3 GetProjectedVector3D(Vector3 const& vector, Vector3 const& basis)
{
	return Vector3::GetProjectedVector(vector, basis);
}

float GetAngleDegreesBetweenVectors2D(Vector2 const& a, Vector2 const& b)
{
	return Vector2::GetAngleDegreesBetween(a, b);
}

int GetTaxicabDistance2D(IntVec2 const& a, IntVec2 const& b) { return abs(a.x - b.x) + abs(a.y - b.y); }

int GetTaxicabDistance2D(Vector2 const& a, Vector2 const& b)
{
	IntVec2 intA(RoundDownToInt(a.x), RoundDownToInt(a.y));
	IntVec2 intB(RoundDownToInt(b.x), RoundDownToInt(b.y));
	return GetTaxicabDistance2D(intA, intB);
}

// --- Is Point Inside ---
bool IsPointInsideDisc2D(Vector2 point, Vector2 discCenter, float discRadius)
{
	Vector2 toPoint     = point - discCenter;
	float   distSquared = toPoint.GetLengthSquared();
	return distSquared < (discRadius * discRadius);
}

bool IsPointInsideDisc2D(Vector2 point, Disc2 const& disc)
{
	return IsPointInsideDisc2D(point, disc.m_center, disc.m_radius);
}

bool IsPointInsideAABB2D(Vector2 point, AABB2 const& alignedBox)
{
	return (
		point.x > alignedBox.m_mins.x && point.x < alignedBox.m_maxs.x && point.y > alignedBox.m_mins.y
		&& point.y < alignedBox.m_maxs.y);
}

bool IsPointInsideAABB3D(Vector3 point, Vector3 const& boxMins, Vector3 const& boxMaxs)
{
	return (
		point.x > boxMins.x && point.x < boxMaxs.x && point.y > boxMins.y && point.y < boxMaxs.y && point.z > boxMins.z
		&& point.z < boxMaxs.z);
}

bool IsPointInsideAABB3D(Vector3 point, AABB3 const& alignedBox)
{
	return IsPointInsideAABB3D(point, alignedBox.m_mins, alignedBox.m_maxs);
}

bool IsPointInsideOBB2D(Vector2 point, OBB2 const& orientedBox)
{
	Vector2 localPos = orientedBox.GetLocalPosForWorldPos(point);
	return IsPointInsideAABB2D(localPos, AABB2(-orientedBox.m_halfDimensions, orientedBox.m_halfDimensions));
}

bool IsPointInsideCapsule2D(Vector2 point, Vector2 boneStart, Vector2 boneEnd, float radius)
{
	Vector2 nearestPoint = GetNearestPointOnLineSegment2D(point, boneStart, boneEnd);
	return IsPointInsideDisc2D(point, nearestPoint, radius);
}

bool IsPointInsideCapsule2D(Vector2 point, Capsule2 const& capsule)
{
	return IsPointInsideCapsule2D(point, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius);
}

bool IsPointInsideTriangle2D(Vector2 point, Vector2 ccw0, Vector2 ccw1, Vector2 ccw2)
{
	Vector2 edge0 = ccw1 - ccw0;
	Vector2 edge1 = ccw2 - ccw1;
	Vector2 edge2 = ccw0 - ccw2;

	Vector2 edge0Rotate90 = edge0.GetRotatedBy90Degrees();
	Vector2 edge1Rotate90 = edge1.GetRotatedBy90Degrees();
	Vector2 edge2Rotate90 = edge2.GetRotatedBy90Degrees();

	Vector2 toPoint0 = point - ccw0;
	Vector2 toPoint1 = point - ccw1;
	Vector2 toPoint2 = point - ccw2;

	if (DotProduct2D(edge0Rotate90, toPoint0) < 0.f)
		return false;
	if (DotProduct2D(edge1Rotate90, toPoint1) < 0.f)
		return false;
	if (DotProduct2D(edge2Rotate90, toPoint2) < 0.f)
		return false;

	return true;
}

bool IsPointInsideTriangle2D(Vector2 point, Triangle2 const& triangle)
{
	return IsPointInsideTriangle2D(
		point,
		triangle.m_pointsCounterClockwise[0],
		triangle.m_pointsCounterClockwise[1],
		triangle.m_pointsCounterClockwise[2]);
}

bool IsPointInsideOrientedSector2D(
	Vector2 point, Vector2 sectorOrigin, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius)
{
	if (!IsPointInsideDisc2D(point, sectorOrigin, sectorRadius))
	{
		return false;
	}

	Vector2 toPoint = point - sectorOrigin;
	Vector2 forward = Vector2::MakeFromPolarDegrees(sectorForwardDegrees, 1.f);
	float   angle   = GetAngleDegreesBetweenVectors2D(toPoint, forward);
	return angle < (sectorApertureDegrees * 0.5f);
}

bool IsPointInsideDirectedSector2D(
	Vector2 point, Vector2 sectorOrigin, Vector2 sectorForwardNormal, float sectorApertureDegrees, float sectorRadius)
{
	if (!IsPointInsideDisc2D(point, sectorOrigin, sectorRadius))
	{
		return false;
	}

	Vector2 toPoint    = point - sectorOrigin;
	Vector2 dirToPoint = toPoint.GetNormalized();
	Vector2 fwd        = sectorForwardNormal.GetNormalized();
	float   cosAngle   = DotProduct2D(dirToPoint, fwd);
	float   cosLimit   = CosDegrees(sectorApertureDegrees * 0.5f);
	return cosAngle > cosLimit;
}

// --- Get Nearest Point On ---
Vector2 GetNearestPointOnDisc2D(Vector2 point, Vector2 discCenter, float discRadius)
{
	Vector2 toPoint = point - discCenter;
	float   dist    = toPoint.GetLength();

	if (dist <= discRadius || dist == 0.f)
	{
		return point;
	}

	return discCenter + toPoint * (discRadius / dist);
}

Vector2 GetNearestPointOnDisc2D(Vector2 referencePos, Disc2 const& disc)
{
	return GetNearestPointOnDisc2D(referencePos, disc.m_center, disc.m_radius);
}

Vector2 GetNearestPointOnAABB2D(Vector2 referencePos, AABB2 const& alignedBox)
{
	return Vector2(
		GetClamped(referencePos.x, alignedBox.m_mins.x, alignedBox.m_maxs.x),
		GetClamped(referencePos.y, alignedBox.m_mins.y, alignedBox.m_maxs.y));
}

Vector2 GetNearestPointOnOBB2D(Vector2 referencePos, OBB2 const& orientedBox)
{
	Vector2 localPos        = orientedBox.GetLocalPosForWorldPos(referencePos);
	Vector2 clampedLocalPos = Vector2(
		GetClamped(localPos.x, -orientedBox.m_halfDimensions.x, orientedBox.m_halfDimensions.x),
		GetClamped(localPos.y, -orientedBox.m_halfDimensions.y, orientedBox.m_halfDimensions.y));
	return orientedBox.GetWorldPosForLocalPos(clampedLocalPos);
}

Vector2 GetNearestPointOnInfiniteLine2D(Vector2 referencePos, Vector2 pointOnLine, Vector2 anotherPointOnLine)
{
	Vector2 lineDir = (anotherPointOnLine - pointOnLine).GetNormalized();

	if (lineDir.GetLengthSquared() == 0.f)
	{
		return pointOnLine;
	}

	Vector2 toReference     = referencePos - pointOnLine;
	float   projectedLength = DotProduct2D(toReference, lineDir);
	return pointOnLine + lineDir * projectedLength;
}

Vector2 GetNearestPointOnInfiniteLine2D(Vector2 referencePos, LineSegment2 const& lineSegmentOnInfiniteLine)
{
	return GetNearestPointOnInfiniteLine2D(
		referencePos,
		lineSegmentOnInfiniteLine.m_start,
		lineSegmentOnInfiniteLine.m_end);
}

Vector2 GetNearestPointOnLineSegment2D(Vector2 referencePos, Vector2 start, Vector2 end)
{
	Vector2 startToEnd = end - start;
	Vector2 endToStart = start - end;

	Vector2 startToRef = referencePos - start;
	Vector2 endToRef   = referencePos - end;

	if (DotProduct2D(startToRef, startToEnd) <= 0.f)
		return start;
	if (DotProduct2D(endToRef, endToStart) <= 0.f)
		return end;

	return GetNearestPointOnInfiniteLine2D(referencePos, start, end);
}

Vector2 GetNearestPointOnLineSegment2D(Vector2 referencePos, LineSegment2 const& lineSegment)
{
	return GetNearestPointOnLineSegment2D(referencePos, lineSegment.m_start, lineSegment.m_end);
}

Vector2 GetNearestPointOnCapsule2D(Vector2 referencePos, Vector2 boneStart, Vector2 boneEnd, float radius)
{
	Vector2 nearestPointOnBone = GetNearestPointOnLineSegment2D(referencePos, boneStart, boneEnd);
	return GetNearestPointOnDisc2D(referencePos, nearestPointOnBone, radius);
}

Vector2 GetNearestPointOnCapsule2D(Vector2 referencePos, Capsule2 const& capsule)
{
	return GetNearestPointOnCapsule2D(referencePos, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius);
}

Vector2 GetNearestPointOnTriangle2D(Vector2 referencePos, Vector2 ccw0, Vector2 ccw1, Vector2 ccw2)
{
	if (IsPointInsideTriangle2D(referencePos, ccw0, ccw1, ccw2))
	{
		return referencePos;
	}

	Vector2 nearest0 = GetNearestPointOnLineSegment2D(referencePos, ccw0, ccw1);
	Vector2 nearest1 = GetNearestPointOnLineSegment2D(referencePos, ccw1, ccw2);
	Vector2 nearest2 = GetNearestPointOnLineSegment2D(referencePos, ccw2, ccw0);

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

Vector2 GetNearestPointOnTriangle2D(Vector2 referencePos, Triangle2 const& triangle)
{
	return GetNearestPointOnTriangle2D(
		referencePos,
		triangle.m_pointsCounterClockwise[0],
		triangle.m_pointsCounterClockwise[1],
		triangle.m_pointsCounterClockwise[2]);
}

Vector3 GetNearestPointOnAABB3D(Vector3 referencePos, AABB3 const& alignedBox)
{
	return Vector3(
		GetClamped(referencePos.x, alignedBox.m_mins.x, alignedBox.m_maxs.x),
		GetClamped(referencePos.y, alignedBox.m_mins.y, alignedBox.m_maxs.y),
		GetClamped(referencePos.z, alignedBox.m_mins.z, alignedBox.m_maxs.z));
}

Vector3 GetNearestPointOnAABB3D(Vector3 referencePos, Vector3 const& boxMins, Vector3 const& boxMaxs)
{
	return Vector3(
		GetClamped(referencePos.x, boxMins.x, boxMaxs.x),
		GetClamped(referencePos.y, boxMins.y, boxMaxs.y),
		GetClamped(referencePos.z, boxMins.z, boxMaxs.z));
}

Vector3 GetNearestPointOnZCylinder3D(Vector3 referencePos, CylinderZ3 const& cylinder)
{
	float   minZ   = cylinder.m_minMaxZ.m_min;
	float   maxZ   = cylinder.m_minMaxZ.m_max;
	float   height = maxZ - minZ;
	Vector3 cylinderStart(cylinder.m_centerXY.x, cylinder.m_centerXY.y, minZ);
	return GetNearestPointOnZCylinder3D(referencePos, cylinderStart, height, cylinder.m_radius);
}

Vector3 GetNearestPointOnZCylinder3D(
	Vector3 referencePos, Vector3 const& cylinderStart, float cylinderHeight, float cylinderRadius)
{
	float const localZ        = referencePos.z - cylinderStart.z;
	float const clampedLocalZ = GetClamped(localZ, 0.f, cylinderHeight);

	Vector3     radialDisplacement(referencePos.x - cylinderStart.x, referencePos.y - cylinderStart.y, 0.f);
	float const radialDistanceSquared = radialDisplacement.GetLengthXYSquared();
	float const radiusSquared         = cylinderRadius * cylinderRadius;

	if (localZ == clampedLocalZ && radialDistanceSquared <= radiusSquared)
	{
		return referencePos;
	}

	Vector3 nearest(cylinderStart.x, cylinderStart.y, cylinderStart.z + clampedLocalZ);
	if (radialDistanceSquared <= radiusSquared)
	{
		nearest.x = referencePos.x;
		nearest.y = referencePos.y;
		return nearest;
	}

	float const radialDistance = sqrtf(radialDistanceSquared);
	if (radialDistance > 0.f)
	{
		float const radialScale = cylinderRadius / radialDistance;
		nearest.x               = cylinderStart.x + radialDisplacement.x * radialScale;
		nearest.y               = cylinderStart.y + radialDisplacement.y * radialScale;
	}

	return nearest;
}

Vector3 GetNearestPointOnSphere3D(Vector3 referencePos, Sphere3 const& sphere)
{
	return GetNearestPointOnSphere3D(referencePos, sphere.m_center, sphere.m_radius);
}

Vector3 GetNearestPointOnSphere3D(Vector3 referencePos, Vector3 const& sphereCenter, float sphereRadius)
{
	Vector3     displacement    = referencePos - sphereCenter;
	float const distanceSquared = displacement.GetLengthSquared();
	float const radiusSquared   = sphereRadius * sphereRadius;
	if (distanceSquared <= radiusSquared)
	{
		return referencePos;
	}

	float const distance = sqrtf(distanceSquared);
	if (distance <= 0.f)
	{
		return sphereCenter;
	}

	Vector3 const outwardNormal = displacement / distance;
	return sphereCenter + outwardNormal * sphereRadius;
}

Vector3 GetNearestPointOnLine3D(Vector3 referencePos, Vector3 const& lineStart, Vector3 const& lineEnd)
{
	if (lineStart == lineEnd)
	{
		return lineStart;
	}

	Vector3 startTooReference = referencePos - lineStart;
	float   lineLength        = (lineEnd - lineStart).GetLength();
	Vector3 lineDir           = (lineEnd - lineStart) / lineLength;
	float   projectedLength   = DotProduct3D(startTooReference, lineDir);

	if (projectedLength <= 0.f)
	{
		return lineStart;
	}
	else if (projectedLength >= lineLength)
	{
		return lineEnd;
	}

	return lineStart + lineDir * projectedLength;
}

Vector3 GetNearestPointOnCapsule3D(Vector3 referencePos, Capsule3 const& capsule)
{
	Vector3 nearestPointOnBone = GetNearestPointOnLine3D(referencePos, capsule.m_start, capsule.m_end);
	return GetNearestPointOnSphere3D(referencePos, nearestPointOnBone, capsule.m_radius);
}

// Use voronoi region-based closest point on triangle algorithm
Vector3 GetNearestPointOnTriangle3D(Vector3 referencePos, Vector3 const& v0, Vector3 const& v1, Vector3 const& v2)
{
	Vector3 ab = v1 - v0;
	Vector3 ac = v2 - v0;
	Vector3 ap = referencePos - v0;

	float d1 = DotProduct3D(ab, ap);
	float d2 = DotProduct3D(ac, ap);

	// closest to v0
	if (d1 <= 0.f && d2 <= 0.f)
	{
		return v0;
	}

	Vector3 bp = referencePos - v1;
	float   d3 = DotProduct3D(ab, bp);
	float   d4 = DotProduct3D(ac, bp);

	// closest to v1
	if (d3 >= 0.f && d4 <= d3)
	{
		return v1;
	}

	// closest on edge v0-v1
	float vc = d1 * d4 - d3 * d2;
	if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
	{
		float t = d1 / (d1 - d3);
		return v0 + t * ab;
	}

	Vector3 cp = referencePos - v2;
	float   d5 = DotProduct3D(ab, cp);
	float   d6 = DotProduct3D(ac, cp);

	// closest to v2
	if (d6 >= 0.f && d5 <= d6)
	{
		return v2;
	}

	// closest on edge v0-v2
	float vb = d5 * d2 - d1 * d6;
	if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
	{
		float t = d2 / (d2 - d6);
		return v0 + t * ac;
	}

	// closest on edge v1-v2
	float va = d3 * d6 - d5 * d4;
	if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
	{
		float t = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return v1 + t * (v2 - v1);
	}

	// inside face region
	float denom = 1.f / (va + vb + vc);
	float v     = vb * denom;
	float w     = vc * denom;

	return v0 + ab * v + ac * w;
}

Vector3 GetNearestPointOnTriangle3D(Vector3 referencePos, Triangle3 const& triangle)
{
	return GetNearestPointOnTriangle3D(
		referencePos,
		triangle.m_pointsCounterClockwise[0],
		triangle.m_pointsCounterClockwise[1],
		triangle.m_pointsCounterClockwise[2]);
}

} // namespace Math

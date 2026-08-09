#include "MingEngine/Core/Math/Vector3.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"

#include <math.h>

using namespace Math;

const Vector3 Vector3::Zero     = Vector3(0.f, 0.f, 0.f);
const Vector3 Vector3::One      = Vector3(1.f, 1.f, 1.f);
const Vector3 Vector3::Forward  = Vector3(1.f, 0.f, 0.f);
const Vector3 Vector3::Backward = Vector3(-1.f, 0.f, 0.f);
const Vector3 Vector3::Left     = Vector3(0.f, 1.f, 0.f);
const Vector3 Vector3::Right    = Vector3(0.f, -1.f, 0.f);
const Vector3 Vector3::Up       = Vector3(0.f, 0.f, 1.f);
const Vector3 Vector3::Down     = Vector3(0.f, 0.f, -1.f);

Vector3::Vector3() = default;

Vector3::Vector3(float initialX, float initialY, float initialZ) : x(initialX), y(initialY), z(initialZ) {}

Vector3::Vector3(Vector3 const& other) = default;

Vector3::Vector3(float initialX, float initialY) : x(initialX), y(initialY), z(0.f) {}

Vector3::Vector3(Vector2 const& other) : x(other.x), y(other.y), z(0.f) {}

Vector3::~Vector3() = default;

Vector3 const Vector3::operator+(Vector3 const& vecToAdd) const
{
	return Vector3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

Vector3 const Vector3::operator-(Vector3 const& vecToSubtract) const
{
	return Vector3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

Vector3 const Vector3::operator-() const { return Vector3(-x, -y, -z); }

Vector3 const Vector3::operator*(float uniformScale) const
{
	return Vector3(x * uniformScale, y * uniformScale, z * uniformScale);
}

Vector3 const Vector3::operator*(Vector3 const& vecToMultiply) const
{
	return Vector3(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z);
}

Vector3 const Vector3::operator/(float inverseScale) const
{
	return Vector3(x / inverseScale, y / inverseScale, z / inverseScale);
}

void Vector3::operator+=(Vector3 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

void Vector3::operator-=(Vector3 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}

void Vector3::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}

void Vector3::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	z /= uniformDivisor;
}

void Vector3::operator=(Vector3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}

Vector3 const operator*(float uniformScale, Vector3 const& vecToScale)
{
	return Vector3(vecToScale.x * uniformScale, vecToScale.y * uniformScale, vecToScale.z * uniformScale);
}

bool Vector3::operator==(Vector3 const& compare) const { return x == compare.x && y == compare.y && z == compare.z; }

bool Vector3::operator!=(Vector3 const& compare) const { return x != compare.x || y != compare.y || z != compare.z; }

float Vector3::GetLength() const { return sqrtf(x * x + y * y + z * z); }

float Vector3::GetLengthXY() const { return sqrtf(x * x + y * y); }

float Vector3::GetLengthSquared() const { return x * x + y * y + z * z; }

float Vector3::GetLengthXYSquared() const { return x * x + y * y; }

float Vector3::GetOrientationAboutZDegrees() const { return ConvertRadiansToDegrees(atan2f(y, x)); }

float Vector3::GetOrientationAboutZRadians() const { return atan2f(y, x); }

Vector3 Vector3::GetRotatedAboutZDegrees(float degrees) const
{
	float radians = ConvertDegreesToRadians(degrees);
	return GetRotatedAboutZRadians(radians);
}

Vector3 Vector3::GetRotatedAboutZRadians(float radians) const
{
	float cosTheta = cosf(radians);
	float sinTheta = sinf(radians);
	return Vector3(x * cosTheta - y * sinTheta, x * sinTheta + y * cosTheta, z);
}

void Vector3::Normalize()
{
	float lenSquared = GetLengthSquared();
	if (lenSquared == 1.f)
	{
		return;
	}

	float len = GetLength();
	if (len > 0.f)
	{
		x /= len;
		y /= len;
		z /= len;
	}
}

Vector3 Vector3::GetNormalized() const
{
	float lenSquared = GetLengthSquared();
	if (lenSquared == 1.f)
	{
		return *this;
	}

	float len = GetLength();
	if (len > 0.f)
	{
		return Vector3(x / len, y / len, z / len);
	}
	else
	{
		return Vector3(0.f, 0.f, 0.f);
	}
}

Vector3 Vector3::MakeFromPolarRadians(float pitchRadians, float yawRadians, float length /*= 1.0f*/)
{
	float cosPitch = cosf(pitchRadians);
	float sinPitch = sinf(pitchRadians);
	float cosYaw   = cosf(yawRadians);
	float sinYaw   = sinf(yawRadians);

	return Vector3(length * cosPitch * cosYaw, length * cosPitch * sinYaw, length * sinPitch);
}

Vector3 Vector3::MakeFromPolarDegrees(float pitchDegrees, float yawDegrees, float length /*= 1.0f*/)
{
	float pitchRadians = pitchDegrees * kDegreesToRadiansMultiplier;
	float yawRadians   = yawDegrees * kDegreesToRadiansMultiplier;
	return MakeFromPolarRadians(pitchRadians, yawRadians, length);
}

float Vector3::DotProduct(Vector3 const& a, Vector3 const& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vector3 Vector3::CrossProduct(Vector3 const& a, Vector3 const& b)
{
	return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

Vector3 Vector3::GetProjectedVector(Vector3 const& vector, Vector3 const& basis)
{
	Vector3 n = basis.GetNormalized();
	return n * DotProduct(vector, n);
}

Vector3 Vector3::Interpolate(Vector3 const& start, Vector3 const& end, float fraction)
{
	return Vector3(
		Math::Interpolate(start.x, end.x, fraction),
		Math::Interpolate(start.y, end.y, fraction),
		Math::Interpolate(start.z, end.z, fraction));
}

Vector3 Vector3::InterpolateClamped(Vector3 const& start, Vector3 const& end, float fraction)
{
	return Interpolate(start, end, GetClampedZeroToOne(fraction));
}

#include "Vec3.hpp"

#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Math/Vec2.hpp"

#include <math.h>

const Vec3 Vec3::Zero     = Vec3(0.f, 0.f, 0.f);
const Vec3 Vec3::One      = Vec3(1.f, 1.f, 1.f);
const Vec3 Vec3::Forward  = Vec3(1.f, 0.f, 0.f);
const Vec3 Vec3::Backward = Vec3(-1.f, 0.f, 0.f);
const Vec3 Vec3::Left     = Vec3(0.f, 1.f, 0.f);
const Vec3 Vec3::Right    = Vec3(0.f, -1.f, 0.f);
const Vec3 Vec3::Up       = Vec3(0.f, 0.f, 1.f);
const Vec3 Vec3::Down     = Vec3(0.f, 0.f, -1.f);

Vec3::Vec3() = default;

Vec3::Vec3(float initialX, float initialY, float initialZ) : x(initialX), y(initialY), z(initialZ) {}

Vec3::Vec3(Vec3 const& other) = default;

Vec3::Vec3(float initialX, float initialY) : x(initialX), y(initialY), z(0.f) {}

Vec3::Vec3(Vec2 const& other) : x(other.x), y(other.y), z(0.f) {}

Vec3::~Vec3() = default;

Vec3 const Vec3::operator+(Vec3 const& vecToAdd) const { return Vec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z); }

Vec3 const Vec3::operator-(Vec3 const& vecToSubtract) const
{
	return Vec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

Vec3 const Vec3::operator-() const { return Vec3(-x, -y, -z); }

Vec3 const Vec3::operator*(float uniformScale) const
{
	return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}

Vec3 const Vec3::operator*(Vec3 const& vecToMultiply) const
{
	return Vec3(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z);
}

Vec3 const Vec3::operator/(float inverseScale) const
{
	return Vec3(x / inverseScale, y / inverseScale, z / inverseScale);
}

void Vec3::operator+=(Vec3 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

void Vec3::operator-=(Vec3 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}

void Vec3::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}

void Vec3::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	z /= uniformDivisor;
}

void Vec3::operator=(Vec3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}

Vec3 const operator*(float uniformScale, Vec3 const& vecToScale)
{
	return Vec3(vecToScale.x * uniformScale, vecToScale.y * uniformScale, vecToScale.z * uniformScale);
}

bool Vec3::operator==(Vec3 const& compare) const { return x == compare.x && y == compare.y && z == compare.z; }

bool Vec3::operator!=(Vec3 const& compare) const { return x != compare.x || y != compare.y || z != compare.z; }

float Vec3::GetLength() const { return sqrtf(x * x + y * y + z * z); }

float Vec3::GetLengthXY() const { return sqrtf(x * x + y * y); }

float Vec3::GetLengthSquared() const { return x * x + y * y + z * z; }

float Vec3::GetLengthXYSquared() const { return x * x + y * y; }

float Vec3::GetOrientationAboutZDegrees() const { return ConvertRadiansToDegrees(atan2f(y, x)); }

float Vec3::GetOrientationAboutZRadians() const { return atan2f(y, x); }

Vec3 Vec3::GetRotatedAboutZDegrees(float degrees) const
{
	float radians = ConvertDegreesToRadians(degrees);
	return GetRotatedAboutZRadians(radians);
}

Vec3 Vec3::GetRotatedAboutZRadians(float radians) const
{
	float cosTheta = cosf(radians);
	float sinTheta = sinf(radians);
	return Vec3(x * cosTheta - y * sinTheta, x * sinTheta + y * cosTheta, z);
}

void Vec3::Normalize()
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

Vec3 Vec3::GetNormalized() const
{
	float lenSquared = GetLengthSquared();
	if (lenSquared == 1.f)
	{
		return *this;
	}

	float len = GetLength();
	if (len > 0.f)
	{
		return Vec3(x / len, y / len, z / len);
	}
	else
	{
		return Vec3(0.f, 0.f, 0.f);
	}
}

Vec3 Vec3::MakeFromPolarRadians(float pitchRadians, float yawRadians, float length /*= 1.0f*/)
{
	float cosPitch = cosf(pitchRadians);
	float sinPitch = sinf(pitchRadians);
	float cosYaw   = cosf(yawRadians);
	float sinYaw   = sinf(yawRadians);

	return Vec3(length * cosPitch * cosYaw, length * cosPitch * sinYaw, length * sinPitch);
}

Vec3 Vec3::MakeFromPolarDegrees(float pitchDegrees, float yawDegrees, float length /*= 1.0f*/)
{
	float pitchRadians = pitchDegrees * kDegreesToRadiansMultiplier;
	float yawRadians   = yawDegrees * kDegreesToRadiansMultiplier;
	return MakeFromPolarRadians(pitchRadians, yawRadians, length);
}

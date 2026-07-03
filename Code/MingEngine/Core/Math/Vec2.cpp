#include "MingEngine/Core/Math/Vec2.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <math.h>

using namespace Math;

const Vec2 Vec2::Zero = Vec2(0.f, 0.f);
const Vec2 Vec2::One  = Vec2(1.f, 1.f);

//-----------------------------------------------------------------------------------------------
Vec2::Vec2() = default;

Vec2::Vec2(Vec2 const& copy) = default;

//-----------------------------------------------------------------------------------------------
Vec2::Vec2(float initialX, float initialY) : x(initialX), y(initialY) {}

Vec2::Vec2(Vec3 const& copyFrom) : x(copyFrom.x), y(copyFrom.y) {}

Vec2::Vec2(IntVec2 const& copyFrom) : x(static_cast<float>(copyFrom.x)), y(static_cast<float>(copyFrom.y)) {}

Vec2::~Vec2() = default;

//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator+(Vec2 const& vecToAdd) const { return Vec2(this->x + vecToAdd.x, this->y + vecToAdd.y); }

//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-(Vec2 const& vecToSubtract) const
{
	return Vec2(this->x - vecToSubtract.x, this->y - vecToSubtract.y);
}

//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-() const { return Vec2(-this->x, -this->y); }

//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*(float uniformScale) const { return Vec2(this->x * uniformScale, this->y * uniformScale); }

//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*(Vec2 const& vecToMultiply) const
{
	return Vec2(this->x * vecToMultiply.x, this->y * vecToMultiply.y);
}

//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator/(float inverseScale) const { return Vec2(this->x / inverseScale, this->y / inverseScale); }

//-----------------------------------------------------------------------------------------------
void Vec2::operator+=(Vec2 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}

//-----------------------------------------------------------------------------------------------
void Vec2::operator-=(Vec2 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}

//-----------------------------------------------------------------------------------------------
void Vec2::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
}

//-----------------------------------------------------------------------------------------------
void Vec2::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}

//-----------------------------------------------------------------------------------------------
void Vec2::operator=(Vec2 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

//-----------------------------------------------------------------------------------------------
Vec2 const operator*(float uniformScale, Vec2 const& vecToScale)
{
	return Vec2(vecToScale.x * uniformScale, vecToScale.y * uniformScale);
}

//-----------------------------------------------------------------------------------------------
bool Vec2::operator==(Vec2 const& compare) const { return x == compare.x && y == compare.y; }

//-----------------------------------------------------------------------------------------------
bool Vec2::operator!=(Vec2 const& compare) const { return x != compare.x || y != compare.y; }

// Static factory methods
Vec2 Vec2::MakeFromPolarDegrees(float degrees, float length)
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	return Vec2(cosf(radians) * length, sinf(radians) * length);
}

Vec2 Vec2::MakeFromPolarRadians(float radians, float length)
{
	return Vec2(cosf(radians) * length, sinf(radians) * length);
}

float Vec2::DotProduct(Vec2 const& a, Vec2 const& b) { return a.x * b.x + a.y * b.y; }

float Vec2::CrossProduct(Vec2 const& a, Vec2 const& b) { return a.x * b.y - a.y * b.x; }

float Vec2::GetProjectedLength(Vec2 const& vector, Vec2 const& basis)
{
	Vec2 n = basis.GetNormalized();
	return DotProduct(vector, n);
}

Vec2 Vec2::GetProjectedVector(Vec2 const& vector, Vec2 const& basis)
{
	Vec2 n = basis.GetNormalized();
	return n * DotProduct(vector, n);
}

float Vec2::GetAngleDegreesBetween(Vec2 const& a, Vec2 const& b)
{
	float aLen = a.GetLength();
	float bLen = b.GetLength();
	if (aLen == 0.f || bLen == 0.f)
		return 0.f;
	float dot = DotProduct(a, b) / (aLen * bLen);
	dot       = Math::GetClamped(dot, -1.f, 1.f);
	return ConvertRadiansToDegrees(acosf(dot));
}

Vec2 Vec2::Interpolate(Vec2 const& start, Vec2 const& end, float fraction)
{
	return Vec2(
		Math::Interpolate(start.x, end.x, fraction),
		Math::Interpolate(start.y, end.y, fraction));
}

Vec2 Vec2::InterpolateClamped(Vec2 const& start, Vec2 const& end, float fraction)
{
	return Interpolate(start, end, GetClampedZeroToOne(fraction));
}

// Getters
float Vec2::GetLength() const { return sqrtf(x * x + y * y); }

float Vec2::GetLengthSquared() const { return x * x + y * y; }

float Vec2::GetOrientationDegrees() const { return atan2f(y, x) * kRadiansToDegreesMultiplier; }

float Vec2::GetOrientationRadians() const { return atan2f(y, x); }

Vec2 Vec2::GetRotatedBy90Degrees() const { return Vec2(-y, x); }

Vec2 Vec2::GetRotatedByMinus90Degrees() const { return Vec2(y, -x); }

Vec2 Vec2::GetRotatedByDegrees(float degrees) const
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	return GetRotatedByRadians(radians);
}

Vec2 Vec2::GetRotatedByRadians(float radians) const
{
	float cosTheta = cosf(radians);
	float sinTheta = sinf(radians);
	return Vec2(x * cosTheta - y * sinTheta, x * sinTheta + y * cosTheta);
}

// Mutators (change this Vec2)
void Vec2::SetOrientationDegrees(float degrees)
{
	float length  = GetLength();
	float radians = degrees * kDegreesToRadiansMultiplier;
	x             = cosf(radians) * length;
	y             = sinf(radians) * length;
}

void Vec2::SetOrientationRadians(float radians)
{
	float length = GetLength();
	x            = cosf(radians) * length;
	y            = sinf(radians) * length;
}

void Vec2::SetPolarDegrees(float degrees, float length)
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	x             = cosf(radians) * length;
	y             = sinf(radians) * length;
}

void Vec2::SetPolarRadians(float radians, float length)
{
	x = cosf(radians) * length;
	y = sinf(radians) * length;
}

void Vec2::RotateDegrees(float degrees)
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	RotateRadians(radians);
}

void Vec2::RotateRadians(float radians)
{
	float cosTheta = cosf(radians);
	float sinTheta = sinf(radians);
	float newX     = x * cosTheta - y * sinTheta;
	float newY     = x * sinTheta + y * cosTheta;
	x              = newX;
	y              = newY;
}

void Vec2::Rotate90Degrees()
{
	float temp = x;
	x          = -y;
	y          = temp;
}

void Vec2::RotateMinus90Degrees()
{
	float temp = x;
	x          = y;
	y          = -temp;
}

// Length/Normalization
Vec2 Vec2::GetClamped(float maxLength) const
{
	float len = GetLength();
	if (len > maxLength && len > 0.f)
	{
		float scale = maxLength / len;
		return Vec2(x * scale, y * scale);
	}
	return *this;
}

Vec2 Vec2::GetNormalized() const
{
	float len = GetLength();
	if (len > 0.f)
	{
		return Vec2(x / len, y / len);
	}
	return Vec2(0.f, 0.f);
}

void Vec2::SetLength(float newLength)
{
	float len = GetLength();
	if (len > 0.f)
	{
		float scale = newLength / len;
		x *= scale;
		y *= scale;
	}
}

void Vec2::ClampLength(float maxLength)
{
	float len = GetLength();
	if (len > maxLength && len > 0.f)
	{
		float scale = maxLength / len;
		x *= scale;
		y *= scale;
	}
}

void Vec2::Normalize()
{
	float len = GetLength();
	if (len > 0.f)
	{
		x /= len;
		y /= len;
	}
}

float Vec2::NormalizeAndGetPreviousLength()
{
	float len = GetLength();
	Normalize();
	return len;
}

Vec2 const Vec2::GetReflected(Vec2 const& normal) const
{
	float dot = DotProduct(*this, normal);
	return (*this - dot * normal) - dot * normal;
}

void Vec2::Reflect(Vec2 const& normal) { *this = GetReflected(normal); }

void Vec2::SetFromText(char const* text)
{
	Strings parts = SplitStringOnDelimiter(std::string(text), ',');

	if (parts.size() < 2)
	{
		x = 0.f;
		y = 0.f;
		return;
	}

	x = (float)atof(parts[0].c_str());
	y = (float)atof(parts[1].c_str());
}

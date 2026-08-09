#include "MingEngine/Core/Math/Vector2.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <math.h>

using namespace Math;

const Vector2 Vector2::Zero = Vector2(0.f, 0.f);
const Vector2 Vector2::One  = Vector2(1.f, 1.f);

//-----------------------------------------------------------------------------------------------
Vector2::Vector2() = default;

Vector2::Vector2(Vector2 const& copy) = default;

//-----------------------------------------------------------------------------------------------
Vector2::Vector2(float initialX, float initialY) : x(initialX), y(initialY) {}

Vector2::Vector2(Vector3 const& copyFrom) : x(copyFrom.x), y(copyFrom.y) {}

Vector2::Vector2(IntVec2 const& copyFrom) : x(static_cast<float>(copyFrom.x)), y(static_cast<float>(copyFrom.y)) {}

Vector2::~Vector2() = default;

//-----------------------------------------------------------------------------------------------
Vector2 const Vector2::operator+(Vector2 const& vecToAdd) const
{
	return Vector2(this->x + vecToAdd.x, this->y + vecToAdd.y);
}

//-----------------------------------------------------------------------------------------------
Vector2 const Vector2::operator-(Vector2 const& vecToSubtract) const
{
	return Vector2(this->x - vecToSubtract.x, this->y - vecToSubtract.y);
}

//------------------------------------------------------------------------------------------------
Vector2 const Vector2::operator-() const { return Vector2(-this->x, -this->y); }

//-----------------------------------------------------------------------------------------------
Vector2 const Vector2::operator*(float uniformScale) const
{
	return Vector2(this->x * uniformScale, this->y * uniformScale);
}

//------------------------------------------------------------------------------------------------
Vector2 const Vector2::operator*(Vector2 const& vecToMultiply) const
{
	return Vector2(this->x * vecToMultiply.x, this->y * vecToMultiply.y);
}

//-----------------------------------------------------------------------------------------------
Vector2 const Vector2::operator/(float inverseScale) const
{
	return Vector2(this->x / inverseScale, this->y / inverseScale);
}

//-----------------------------------------------------------------------------------------------
void Vector2::operator+=(Vector2 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}

//-----------------------------------------------------------------------------------------------
void Vector2::operator-=(Vector2 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}

//-----------------------------------------------------------------------------------------------
void Vector2::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
}

//-----------------------------------------------------------------------------------------------
void Vector2::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}

//-----------------------------------------------------------------------------------------------
void Vector2::operator=(Vector2 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

//-----------------------------------------------------------------------------------------------
Vector2 const operator*(float uniformScale, Vector2 const& vecToScale)
{
	return Vector2(vecToScale.x * uniformScale, vecToScale.y * uniformScale);
}

//-----------------------------------------------------------------------------------------------
bool Vector2::operator==(Vector2 const& compare) const { return x == compare.x && y == compare.y; }

//-----------------------------------------------------------------------------------------------
bool Vector2::operator!=(Vector2 const& compare) const { return x != compare.x || y != compare.y; }

// Static factory methods
Vector2 Vector2::MakeFromPolarDegrees(float degrees, float length)
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	return Vector2(cosf(radians) * length, sinf(radians) * length);
}

Vector2 Vector2::MakeFromPolarRadians(float radians, float length)
{
	return Vector2(cosf(radians) * length, sinf(radians) * length);
}

float Vector2::DotProduct(Vector2 const& a, Vector2 const& b) { return a.x * b.x + a.y * b.y; }

float Vector2::CrossProduct(Vector2 const& a, Vector2 const& b) { return a.x * b.y - a.y * b.x; }

float Vector2::GetProjectedLength(Vector2 const& vector, Vector2 const& basis)
{
	Vector2 n = basis.GetNormalized();
	return DotProduct(vector, n);
}

Vector2 Vector2::GetProjectedVector(Vector2 const& vector, Vector2 const& basis)
{
	Vector2 n = basis.GetNormalized();
	return n * DotProduct(vector, n);
}

float Vector2::GetAngleDegreesBetween(Vector2 const& a, Vector2 const& b)
{
	float aLen = a.GetLength();
	float bLen = b.GetLength();
	if (aLen == 0.f || bLen == 0.f)
		return 0.f;
	float dot = DotProduct(a, b) / (aLen * bLen);
	dot       = Math::GetClamped(dot, -1.f, 1.f);
	return ConvertRadiansToDegrees(acosf(dot));
}

Vector2 Vector2::Interpolate(Vector2 const& start, Vector2 const& end, float fraction)
{
	return Vector2(Math::Interpolate(start.x, end.x, fraction), Math::Interpolate(start.y, end.y, fraction));
}

Vector2 Vector2::InterpolateClamped(Vector2 const& start, Vector2 const& end, float fraction)
{
	return Interpolate(start, end, GetClampedZeroToOne(fraction));
}

// Getters
float Vector2::GetLength() const { return sqrtf(x * x + y * y); }

float Vector2::GetLengthSquared() const { return x * x + y * y; }

float Vector2::GetOrientationDegrees() const { return atan2f(y, x) * kRadiansToDegreesMultiplier; }

float Vector2::GetOrientationRadians() const { return atan2f(y, x); }

Vector2 Vector2::GetRotatedBy90Degrees() const { return Vector2(-y, x); }

Vector2 Vector2::GetRotatedByMinus90Degrees() const { return Vector2(y, -x); }

Vector2 Vector2::GetRotatedByDegrees(float degrees) const
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	return GetRotatedByRadians(radians);
}

Vector2 Vector2::GetRotatedByRadians(float radians) const
{
	float cosTheta = cosf(radians);
	float sinTheta = sinf(radians);
	return Vector2(x * cosTheta - y * sinTheta, x * sinTheta + y * cosTheta);
}

// Mutators (change this Vec2)
void Vector2::SetOrientationDegrees(float degrees)
{
	float length  = GetLength();
	float radians = degrees * kDegreesToRadiansMultiplier;
	x             = cosf(radians) * length;
	y             = sinf(radians) * length;
}

void Vector2::SetOrientationRadians(float radians)
{
	float length = GetLength();
	x            = cosf(radians) * length;
	y            = sinf(radians) * length;
}

void Vector2::SetPolarDegrees(float degrees, float length)
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	x             = cosf(radians) * length;
	y             = sinf(radians) * length;
}

void Vector2::SetPolarRadians(float radians, float length)
{
	x = cosf(radians) * length;
	y = sinf(radians) * length;
}

void Vector2::RotateDegrees(float degrees)
{
	float radians = degrees * kDegreesToRadiansMultiplier;
	RotateRadians(radians);
}

void Vector2::RotateRadians(float radians)
{
	float cosTheta = cosf(radians);
	float sinTheta = sinf(radians);
	float newX     = x * cosTheta - y * sinTheta;
	float newY     = x * sinTheta + y * cosTheta;
	x              = newX;
	y              = newY;
}

void Vector2::Rotate90Degrees()
{
	float temp = x;
	x          = -y;
	y          = temp;
}

void Vector2::RotateMinus90Degrees()
{
	float temp = x;
	x          = y;
	y          = -temp;
}

// Length/Normalization
Vector2 Vector2::GetClamped(float maxLength) const
{
	float len = GetLength();
	if (len > maxLength && len > 0.f)
	{
		float scale = maxLength / len;
		return Vector2(x * scale, y * scale);
	}
	return *this;
}

Vector2 Vector2::GetNormalized() const
{
	float len = GetLength();
	if (len > 0.f)
	{
		return Vector2(x / len, y / len);
	}
	return Vector2(0.f, 0.f);
}

void Vector2::SetLength(float newLength)
{
	float len = GetLength();
	if (len > 0.f)
	{
		float scale = newLength / len;
		x *= scale;
		y *= scale;
	}
}

void Vector2::ClampLength(float maxLength)
{
	float len = GetLength();
	if (len > maxLength && len > 0.f)
	{
		float scale = maxLength / len;
		x *= scale;
		y *= scale;
	}
}

void Vector2::Normalize()
{
	float len = GetLength();
	if (len > 0.f)
	{
		x /= len;
		y /= len;
	}
}

float Vector2::NormalizeAndGetPreviousLength()
{
	float len = GetLength();
	Normalize();
	return len;
}

Vector2 const Vector2::GetReflected(Vector2 const& normal) const
{
	float dot = DotProduct(*this, normal);
	return (*this - dot * normal) - dot * normal;
}

void Vector2::Reflect(Vector2 const& normal) { *this = GetReflected(normal); }

void Vector2::SetFromText(char const* text)
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

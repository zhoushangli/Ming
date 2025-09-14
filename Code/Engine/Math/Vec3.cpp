#include "Vec3.hpp"

#include <cmath>

#include "Vec2.hpp"

constexpr float RadiansToDegreesMultiplier = 57.29577951f;
constexpr float DegreesToRadiansMultiplier = 0.01745329252f;

Vec3::Vec3() = default;

Vec3::Vec3(float initialX, float initialY, float initialZ)
	: x(initialX), y(initialY), z(initialZ)
{
}

Vec3::Vec3(Vec3 const& other) = default;

Vec3::Vec3(float initialX, float initialY) : x(initialX), y(initialY), z(0.f)
{
}

Vec3::Vec3(Vec2 const& other) : x(other.x), y(other.y), z(0.f)
{
}

Vec3::~Vec3() = default;

//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator+ (Vec3 const& vecToAdd) const
{
	return Vec3(this->x + vecToAdd.x, this->y + vecToAdd.y, this->z + vecToAdd.z);
}


//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator-(Vec3 const& vecToSubtract) const
{
	return Vec3(this->x - vecToSubtract.x, this->y - vecToSubtract.y, this->z - vecToSubtract.z);
}


//------------------------------------------------------------------------------------------------
Vec3 const Vec3::operator-() const
{
	return Vec3(-this->x, -this->y, -this->z);
}


//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator*(float uniformScale) const
{
	return Vec3(this->x * uniformScale, this->y * uniformScale, this->z * uniformScale);
}


//------------------------------------------------------------------------------------------------
Vec3 const Vec3::operator*(Vec3 const& vecToMultiply) const
{
	return Vec3(this->x * vecToMultiply.x, this->y * vecToMultiply.y, this->z * vecToMultiply.z);
}


//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator/(float inverseScale) const
{
	return Vec3(this->x / inverseScale, this->y / inverseScale, this->z / inverseScale);
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator+=(Vec3 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator-=(Vec3 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	z /= uniformDivisor;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator=(Vec3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}


//-----------------------------------------------------------------------------------------------
Vec3 const operator*(float uniformScale, Vec3 const& vecToScale)
{
	return Vec3(vecToScale.x * uniformScale, vecToScale.y * uniformScale, vecToScale.z * uniformScale);
}


//-----------------------------------------------------------------------------------------------
bool Vec3::operator==(Vec3 const& compare) const
{
	return x == compare.x && y == compare.y && z == compare.z;
}


//-----------------------------------------------------------------------------------------------
bool Vec3::operator!=(Vec3 const& compare) const
{
	return x != compare.x || y != compare.y || z != compare.z;
}

float Vec3::GetLength() const
{
	return std::sqrt(x * x + y * y + z * z);
}

float Vec3::GetLengthXY() const
{
	return std::sqrt(x * x + y * y);
}

float Vec3::GetLengthSquared() const
{
	return x * x + y * y + z * z;
}

float Vec3::GetLengthXYSquared() const
{
	return x * x + y * y;
}

float Vec3::GetOrientationAboutZDegrees() const
{
	return std::atan2(y, x) * RadiansToDegreesMultiplier;
}

float Vec3::GetOrientationAboutZRadians() const
{
	return std::atan2(y, x);
}

Vec3 Vec3::GetRotatedAboutZDegrees(float degrees) const
{
	float radians = degrees * DegreesToRadiansMultiplier;
	return GetRotatedAboutZRadians(radians);
}

Vec3 Vec3::GetRotatedAboutZRadians(float radians) const
{
	float cosTheta = std::cos(radians);
	float sinTheta = std::sin(radians);
	return Vec3(
		x * cosTheta - y * sinTheta,
		x * sinTheta + y * cosTheta,
		z
	);
}
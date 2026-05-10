#include "Engine/Math/Vec4.hpp"

#include <math.h>

const Vec4 Vec4::Zero = Vec4(0.f, 0.f, 0.f, 0.f);
const Vec4 Vec4::One  = Vec4(1.f, 1.f, 1.f, 1.f);

Vec4::Vec4()                  = default;
Vec4::Vec4(Vec4 const& other) = default;
Vec4::~Vec4()                 = default;

Vec4::Vec4(float xVal, float yVal, float zVal, float wVal) : x(xVal), y(yVal), z(zVal), w(wVal) {}

bool Vec4::operator==(Vec4 const& compare) const
{
	return x == compare.x && y == compare.y && z == compare.z && w == compare.w;
}

bool Vec4::operator!=(Vec4 const& compare) const
{
	return x != compare.x || y != compare.y || z != compare.z || w != compare.w;
}

Vec4 const Vec4::operator+(Vec4 const& vecToAdd) const
{
	return Vec4(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z, w + vecToAdd.w);
}

Vec4 const Vec4::operator-(Vec4 const& vecToSubtract) const
{
	return Vec4(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z, w - vecToSubtract.w);
}

Vec4 const Vec4::operator-() const { return Vec4(-x, -y, -z, -w); }

Vec4 const Vec4::operator*(float uniformScale) const
{
	return Vec4(x * uniformScale, y * uniformScale, z * uniformScale, w * uniformScale);
}

Vec4 const Vec4::operator*(Vec4 const& vecToMultiply) const
{
	return Vec4(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z, w * vecToMultiply.w);
}

Vec4 const Vec4::operator/(float inverseScale) const
{
	return Vec4(x / inverseScale, y / inverseScale, z / inverseScale, w / inverseScale);
}

void Vec4::operator+=(Vec4 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
	w += vecToAdd.w;
}

void Vec4::operator-=(Vec4 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
	w -= vecToSubtract.w;
}

void Vec4::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
	w *= uniformScale;
}

void Vec4::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	z /= uniformDivisor;
	w /= uniformDivisor;
}

void Vec4::operator=(Vec4 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
	w = copyFrom.w;
}

Vec4 const operator*(float uniformScale, Vec4 const& vecToScale)
{
	return Vec4(
		vecToScale.x * uniformScale,
		vecToScale.y * uniformScale,
		vecToScale.z * uniformScale,
		vecToScale.w * uniformScale
	);
}

float Vec4::GetLength() const { return sqrtf(GetLengthSquared()); }

float Vec4::GetLengthSquared() const { return x * x + y * y + z * z + w * w; }

void Vec4::Normalize() { *this /= GetLength(); }

Vec4 Vec4::GetNormalized() const { return *this / GetLength(); }

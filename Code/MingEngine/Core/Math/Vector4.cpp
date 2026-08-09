#include "MingEngine/Core/Math/Vector4.hpp"

#include <math.h>

const Vector4 Vector4::Zero = Vector4(0.f, 0.f, 0.f, 0.f);
const Vector4 Vector4::One  = Vector4(1.f, 1.f, 1.f, 1.f);

Vector4::Vector4()                     = default;
Vector4::Vector4(Vector4 const& other) = default;
Vector4::~Vector4()                    = default;

Vector4::Vector4(float xVal, float yVal, float zVal, float wVal) : x(xVal), y(yVal), z(zVal), w(wVal) {}

bool Vector4::operator==(Vector4 const& compare) const
{
	return x == compare.x && y == compare.y && z == compare.z && w == compare.w;
}

bool Vector4::operator!=(Vector4 const& compare) const
{
	return x != compare.x || y != compare.y || z != compare.z || w != compare.w;
}

Vector4 const Vector4::operator+(Vector4 const& vecToAdd) const
{
	return Vector4(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z, w + vecToAdd.w);
}

Vector4 const Vector4::operator-(Vector4 const& vecToSubtract) const
{
	return Vector4(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z, w - vecToSubtract.w);
}

Vector4 const Vector4::operator-() const { return Vector4(-x, -y, -z, -w); }

Vector4 const Vector4::operator*(float uniformScale) const
{
	return Vector4(x * uniformScale, y * uniformScale, z * uniformScale, w * uniformScale);
}

Vector4 const Vector4::operator*(Vector4 const& vecToMultiply) const
{
	return Vector4(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z, w * vecToMultiply.w);
}

Vector4 const Vector4::operator/(float inverseScale) const
{
	return Vector4(x / inverseScale, y / inverseScale, z / inverseScale, w / inverseScale);
}

void Vector4::operator+=(Vector4 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
	w += vecToAdd.w;
}

void Vector4::operator-=(Vector4 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
	w -= vecToSubtract.w;
}

void Vector4::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
	w *= uniformScale;
}

void Vector4::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	z /= uniformDivisor;
	w /= uniformDivisor;
}

void Vector4::operator=(Vector4 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
	w = copyFrom.w;
}

Vector4 const operator*(float uniformScale, Vector4 const& vecToScale)
{
	return Vector4(
		vecToScale.x * uniformScale,
		vecToScale.y * uniformScale,
		vecToScale.z * uniformScale,
		vecToScale.w * uniformScale);
}

float Vector4::GetLength() const { return sqrtf(GetLengthSquared()); }

float Vector4::GetLengthSquared() const { return x * x + y * y + z * z + w * w; }

void Vector4::Normalize() { *this /= GetLength(); }

Vector4 Vector4::GetNormalized() const { return *this / GetLength(); }

float Vector4::DotProduct(Vector4 const& a, Vector4 const& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

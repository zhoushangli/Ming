#include "MingEngine/Core/Math/IntVec2.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <math.h>

using namespace Math;

const IntVec2 IntVec2::Zero = IntVec2(0, 0);

IntVec2::IntVec2(int initialX, int initialY) : x(initialX), y(initialY) {}

IntVec2::IntVec2(const Vector2& copyFrom) : x(RoundDownToInt(copyFrom.x)), y(RoundDownToInt(copyFrom.y)) {}

float IntVec2::GetLength() const { return sqrtf(static_cast<float>(GetLengthSquared())); }

int IntVec2::GetLengthSquared() const { return (x * x) + (y * y); }

int IntVec2::GetTaxicabLength() const { return abs(x) + abs(y); }

float IntVec2::GetOrientationRadians() const { return atan2f((float)y, (float)x); }

float IntVec2::GetOrientationDegrees() const { return ConvertRadiansToDegrees(GetOrientationRadians()); }

IntVec2 IntVec2::GetRotatedBy90Degrees() const { return IntVec2(-y, x); }

IntVec2 IntVec2::GetRotatedByMinus90Degrees() const { return IntVec2(y, -x); }

void IntVec2::Rotate90Degrees()
{
	int oldX = x;
	x        = -y;
	y        = oldX;
}

void IntVec2::RotateMinus90Degrees()
{
	int oldX = x;
	x        = y;
	y        = -oldX;
}

bool IntVec2::operator==(const IntVec2& other) const { return (x == other.x) && (y == other.y); }

bool IntVec2::operator!=(IntVec2 const& compare) const { return !(*this == compare); }

IntVec2 const IntVec2::operator+(IntVec2 const& vecToAdd) const { return IntVec2(x + vecToAdd.x, y + vecToAdd.y); }

IntVec2 const IntVec2::operator-(IntVec2 const& vecToSubtract) const
{
	return IntVec2(x - vecToSubtract.x, y - vecToSubtract.y);
}

IntVec2 const IntVec2::operator-() const { return IntVec2(-x, -y); }

IntVec2 const IntVec2::operator*(float uniformScale) const
{
	return IntVec2(RoundDownToInt((float)x * uniformScale), RoundDownToInt((float)y * uniformScale));
}

IntVec2 const IntVec2::operator*(IntVec2 const& vecToMultiply) const
{
	return IntVec2(x * vecToMultiply.x, y * vecToMultiply.y);
}

IntVec2 const IntVec2::operator/(float inverseScale) const
{
	return IntVec2(RoundDownToInt((float)x / inverseScale), RoundDownToInt((float)y / inverseScale));
}

void IntVec2::operator+=(IntVec2 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}

void IntVec2::operator-=(IntVec2 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}

void IntVec2::operator*=(const float uniformScale)
{
	x = RoundDownToInt((float)x * uniformScale);
	y = RoundDownToInt((float)y * uniformScale);
}

void IntVec2::operator/=(const float uniformDivisor)
{
	x = RoundDownToInt((float)x / uniformDivisor);
	y = RoundDownToInt((float)y / uniformDivisor);
}

void IntVec2::operator=(IntVec2 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

IntVec2::operator Vector2() const { return Vector2(static_cast<float>(x), static_cast<float>(y)); }

void IntVec2::SetFromText(char const* text)
{
	Strings parts = SplitStringOnDelimiter(std::string(text), ',');

	if (parts.size() != 2)
	{
		x = 0;
		y = 0;
		return;
	}

	x = atoi(parts[0].c_str());
	y = atoi(parts[1].c_str());
}

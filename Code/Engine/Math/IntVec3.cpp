#include "Engine/Math/IntVec3.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"

#include <string>

IntVec3 const IntVec3::ZERO = IntVec3(0, 0, 0);

IntVec3::IntVec3(Vec3 const& copyFrom)
	: x(RoundDownToInt(copyFrom.x)), y(RoundDownToInt(copyFrom.y)), z(RoundDownToInt(copyFrom.z))
{
}

IntVec3::IntVec3(int initialX, int initialY, int initialZ) : x(initialX), y(initialY), z(initialZ) {}

void IntVec3::SetFromText(char const* text)
{
	GUARANTEE_OR_DIE(text != nullptr, "IntVec3 text must not be null");

	std::string const value(text);
	size_t const      firstComma  = value.find(',');
	size_t const      secondComma = value.find(',', firstComma == std::string::npos ? firstComma : firstComma + 1);
	GUARANTEE_OR_DIE(
		firstComma != std::string::npos && secondComma != std::string::npos && value.find(',', secondComma + 1) == std::string::npos,
		"IntVec3 text must be formatted as x,y,z"
	);

	x = std::stoi(value.substr(0, firstComma));
	y = std::stoi(value.substr(firstComma + 1, secondComma - firstComma - 1));
	z = std::stoi(value.substr(secondComma + 1));
}

bool IntVec3::operator==(IntVec3 const& compare) const { return x == compare.x && y == compare.y && z == compare.z; }

bool IntVec3::operator!=(IntVec3 const& compare) const { return !(*this == compare); }

IntVec3 IntVec3::operator+(IntVec3 const& vecToAdd) const { return IntVec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z); }

IntVec3 IntVec3::operator-(IntVec3 const& vecToSubtract) const
{
	return IntVec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

void IntVec3::operator+=(IntVec3 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

void IntVec3::operator-=(IntVec3 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}

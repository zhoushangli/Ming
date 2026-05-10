#pragma once

struct Vec3;

struct IntVec3
{
public:
	IntVec3()                        = default;
	IntVec3(IntVec3 const& copyFrom) = default;
	explicit IntVec3(Vec3 const& copyFrom);
	explicit IntVec3(int initialX, int initialY, int initialZ);

	void SetFromText(char const* text);

	bool    operator==(IntVec3 const& compare) const;
	bool    operator!=(IntVec3 const& compare) const;
	IntVec3 operator+(IntVec3 const& vecToAdd) const;
	IntVec3 operator-(IntVec3 const& vecToSubtract) const;
	void    operator+=(IntVec3 const& vecToAdd);
	void    operator-=(IntVec3 const& vecToSubtract);

public:
	int x = 0;
	int y = 0;
	int z = 0;

	static IntVec3 const Zero;
};

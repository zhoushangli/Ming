#pragma once

#include "Engine/Math/Vec2.hpp"

class IntVec2
{
public:
    IntVec2() = default;
	~IntVec2() = default;
	IntVec2(const IntVec2& copyFrom) = default;
    explicit IntVec2(int initialX, int initialY);

    float GetLength() const;
    int GetLengthSquared() const;
    int GetTaxicabLength() const;
    float GetOrientationRadians() const;
    float GetOrientationDegrees() const;
    IntVec2 GetRotatedBy90Degrees() const;
    IntVec2 GetRotatedByMinus90Degrees() const;
    void Rotate90Degrees();
    void RotateMinus90Degrees();

	// Operators (const)
	bool			operator==(IntVec2 const& compare) const;	
	bool			operator!=(IntVec2 const& compare) const;		
	IntVec2 const	operator+(IntVec2 const& vecToAdd) const;		
	IntVec2 const	operator-(IntVec2 const& vecToSubtract) const;	
	IntVec2 const	operator-() const;							
	IntVec2 const	operator*(float uniformScale) const;		
	IntVec2 const	operator*(IntVec2 const& vecToMultiply) const;	
	IntVec2 const	operator/(float inverseScale) const;		

	// Operators (self-mutating / non-const)
	void			operator+=(IntVec2 const& vecToAdd);
	void			operator-=(IntVec2 const& vecToSubtract);
	void			operator*=(const float uniformScale);
	void			operator/=(const float uniformDivisor);
	void			operator=(IntVec2 const& copyFrom);

	operator Vec2() const;

    void SetFromText(char const* text);

public:
	int x = 0;
	int y = 0;

	static const IntVec2 ZERO;
};
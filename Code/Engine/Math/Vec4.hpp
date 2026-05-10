#pragma once

struct Vec4
{
public:
	// Construction/Destruction
	Vec4();
	Vec4(Vec4 const& other);
	~Vec4();
	explicit Vec4(float xVal, float yVal, float zVal, float wVal);

	// Operators (const)
	bool       operator==(Vec4 const& compare) const;
	bool       operator!=(Vec4 const& compare) const;
	Vec4 const operator+(Vec4 const& vecToAdd) const;
	Vec4 const operator-(Vec4 const& vecToSubtract) const;
	Vec4 const operator-() const;
	Vec4 const operator*(float uniformScale) const;
	Vec4 const operator*(Vec4 const& vecToMultiply) const;
	Vec4 const operator/(float inverseScale) const;

	// Operators (self-mutating / non-const)
	void operator+=(Vec4 const& vecToAdd);
	void operator-=(Vec4 const& vecToSubtract);
	void operator*=(const float uniformScale);
	void operator/=(const float uniformDivisor);
	void operator=(Vec4 const& copyFrom);

	// Standalone "friend" functions
	friend Vec4 const operator*(float uniformScale, Vec4 const& vecToScale);

	// Vec4 methods
	float GetLength() const;
	float GetLengthSquared() const;

	void Normalize();
	Vec4 GetNormalized() const;

public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;

	static const Vec4 Zero;
	static const Vec4 One;
};
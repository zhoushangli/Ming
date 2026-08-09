#pragma once

struct Vector4
{
public:
	// Construction/Destruction
	Vector4();
	Vector4(Vector4 const& other);
	~Vector4();
	explicit Vector4(float xVal, float yVal, float zVal, float wVal);

	// Operators (const)
	bool          operator==(Vector4 const& compare) const;
	bool          operator!=(Vector4 const& compare) const;
	Vector4 const operator+(Vector4 const& vecToAdd) const;
	Vector4 const operator-(Vector4 const& vecToSubtract) const;
	Vector4 const operator-() const;
	Vector4 const operator*(float uniformScale) const;
	Vector4 const operator*(Vector4 const& vecToMultiply) const;
	Vector4 const operator/(float inverseScale) const;

	// Operators (self-mutating / non-const)
	void operator+=(Vector4 const& vecToAdd);
	void operator-=(Vector4 const& vecToSubtract);
	void operator*=(const float uniformScale);
	void operator/=(const float uniformDivisor);
	void operator=(Vector4 const& copyFrom);

	// Standalone "friend" functions
	friend Vector4 const operator*(float uniformScale, Vector4 const& vecToScale);

	// Vec4 methods
	float GetLength() const;
	float GetLengthSquared() const;

	void    Normalize();
	Vector4 GetNormalized() const;

	static float DotProduct(Vector4 const& a, Vector4 const& b);

public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;

	static const Vector4 Zero;
	static const Vector4 One;
};

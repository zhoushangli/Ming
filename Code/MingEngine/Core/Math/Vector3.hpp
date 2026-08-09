#pragma once

struct Vector2;

struct Vector3
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

public:
	Vector3();
	Vector3(Vector3 const& other);
	Vector3(Vector2 const& other);
	~Vector3();
	explicit Vector3(float x, float y, float z);
	explicit Vector3(float initialX, float initialY);

	// Operators (const)
	bool          operator==(Vector3 const& compare) const;
	bool          operator!=(Vector3 const& compare) const;
	Vector3 const operator+(Vector3 const& vecToAdd) const;
	Vector3 const operator-(Vector3 const& vecToSubtract) const;
	Vector3 const operator-() const;
	Vector3 const operator*(float uniformScale) const;
	Vector3 const operator*(Vector3 const& vecToMultiply) const;
	Vector3 const operator/(float inverseScale) const;

	// Operators (self-mutating / non-const)
	void operator+=(Vector3 const& vecToAdd);
	void operator-=(Vector3 const& vecToSubtract);
	void operator*=(const float uniformScale);
	void operator/=(const float uniformDivisor);
	void operator=(Vector3 const& copyFrom);

	// Standalone "friend" functions
	friend Vector3 const operator*(float uniformScale, Vector3 const& vecToScale);

	// Vector3 methods
	float   GetLength() const;
	float   GetLengthXY() const;
	float   GetLengthSquared() const;
	float   GetLengthXYSquared() const;
	float   GetOrientationAboutZDegrees() const;
	float   GetOrientationAboutZRadians() const;
	Vector3 GetRotatedAboutZDegrees(float degrees) const;
	Vector3 GetRotatedAboutZRadians(float radians) const;

	void    Normalize();
	Vector3 GetNormalized() const;

	static Vector3 MakeFromPolarRadians(float pitchRadians, float yawRadians, float length = 1.0f);
	static Vector3 MakeFromPolarDegrees(float pitchDegrees, float yawDegrees, float length = 1.0f);
	static float   DotProduct(Vector3 const& a, Vector3 const& b);
	static Vector3 CrossProduct(Vector3 const& a, Vector3 const& b);
	static Vector3 GetProjectedVector(Vector3 const& vector, Vector3 const& basis);
	static Vector3 Interpolate(Vector3 const& start, Vector3 const& end, float fraction);
	static Vector3 InterpolateClamped(Vector3 const& start, Vector3 const& end, float fraction);

	static const Vector3 Zero;
	static const Vector3 One;
	static const Vector3 Forward;
	static const Vector3 Backward;
	static const Vector3 Left;
	static const Vector3 Right;
	static const Vector3 Up;
	static const Vector3 Down;
};

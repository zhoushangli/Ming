#pragma once

struct Vec3
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f; // test

public:
	Vec3();
	Vec3(float x, float y, float z);
	Vec3(Vec3 const& other);
	~Vec3();
	explicit Vec3(float initialX, float initialY);

	// Operators (const)
	bool		operator==(Vec3 const& compare) const;
	bool		operator!=(Vec3 const& compare) const;
	Vec3 const	operator+(Vec3 const& vecToAdd) const;
	Vec3 const	operator-(Vec3 const& vecToSubtract) const;
	Vec3 const	operator-() const;
	Vec3 const	operator*(float uniformScale) const;
	Vec3 const	operator*(Vec3 const& vecToMultiply) const;
	Vec3 const	operator/(float inverseScale) const;

	// Operators (self-mutating / non-const)
	void		operator+=(Vec3 const& vecToAdd);
	void		operator-=(Vec3 const& vecToSubtract);
	void		operator*=(const float uniformScale);
	void		operator/=(const float uniformDivisor);
	void		operator=(Vec3 const& copyFrom);

	// Standalone "friend" functions
	friend Vec3 const operator*(float uniformScale, Vec3 const& vecToScale);

	// Vec3 methods
	float		GetLength() const;
	float		GetLengthXY() const;
	float		GetLengthSquared() const;
	float		GetLengthXYSquared() const;
	float		GetOrientationAboutZDegrees() const;
	float		GetOrientationAboutZRadians() const;
	Vec3		GetRotatedAboutZDegrees(float degrees) const;
	Vec3		GetRotatedAboutZRadians(float radians) const;
};


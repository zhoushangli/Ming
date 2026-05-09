#pragma once

struct Vec2;

struct Vec3
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

public:
	Vec3();
	Vec3(Vec3 const& other);
	Vec3(Vec2 const& other);
	~Vec3();
	explicit Vec3(float x, float y, float z);
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

    void        Normalize();
    Vec3		GetNormalized() const;

    static Vec3 MakeFromPolarRadians(float pitchRadians, float yawRadians, float length = 1.0f);
    static Vec3 MakeFromPolarDegrees(float pitchDegrees, float yawDegrees, float length = 1.0f);

    static const Vec3 kZero;
	static const Vec3 kOne;
    static const Vec3 kForward;
    static const Vec3 kBackward;
    static const Vec3 kLeft;
    static const Vec3 kRight;
    static const Vec3 kUp;
    static const Vec3 kDown;
};


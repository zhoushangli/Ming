#pragma once

struct IntVec2;
struct Vector3;

//-----------------------------------------------------------------------------------------------
struct Vector2
{
public:
	// Construction/Destruction
	~Vector2();                                       // destructor (do nothing)
	Vector2();                                        // default constructor (do nothing)
	Vector2(Vector2 const& copyFrom);                 // copy constructor (from another vec2)
	Vector2(Vector3 const& copyFrom);                 // copy constructor (from Vec3, dropping Z)
	Vector2(IntVec2 const& copyFrom);                 // copy constructor (from IntVec2)
	explicit Vector2(float initialX, float initialY); // explicit constructor (from x, y)

	// Operators (const)
	bool          operator==(Vector2 const& compare) const;      // vec2 == vec2
	bool          operator!=(Vector2 const& compare) const;      // vec2 != vec2
	Vector2 const operator+(Vector2 const& vecToAdd) const;      // vec2 + vec2
	Vector2 const operator-(Vector2 const& vecToSubtract) const; // vec2 - vec2
	Vector2 const operator-() const;                             // -vec2, i.e. "unary negation"
	Vector2 const operator*(float uniformScale) const;           // vec2 * float
	Vector2 const operator*(Vector2 const& vecToMultiply) const; // vec2 * vec2
	Vector2 const operator/(float inverseScale) const;           // vec2 / float

	// Operators (self-mutating / non-const)
	void operator+=(Vector2 const& vecToAdd);      // vec2 += vec2
	void operator-=(Vector2 const& vecToSubtract); // vec2 -= vec2
	void operator*=(const float uniformScale);     // vec2 *= float
	void operator/=(const float uniformDivisor);   // vec2 /= float
	void operator=(Vector2 const& copyFrom);       // vec2 = vec2

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec2::
	friend Vector2 const operator*(float uniformScale, Vector2 const& vecToScale); // float * vec2

	// Static factory methods
	static Vector2 MakeFromPolarDegrees(float degrees, float length = 1.f);
	static Vector2 MakeFromPolarRadians(float radians, float length = 1.f);
	static float   DotProduct(Vector2 const& a, Vector2 const& b);
	static float   CrossProduct(Vector2 const& a, Vector2 const& b);
	static float   GetProjectedLength(Vector2 const& vector, Vector2 const& basis);
	static Vector2 GetProjectedVector(Vector2 const& vector, Vector2 const& basis);
	static float   GetAngleDegreesBetween(Vector2 const& a, Vector2 const& b);
	static Vector2 Interpolate(Vector2 const& start, Vector2 const& end, float fraction);
	static Vector2 InterpolateClamped(Vector2 const& start, Vector2 const& end, float fraction);

	// Getters
	float GetLength() const;
	float GetLengthSquared() const;
	float GetOrientationDegrees() const;
	float GetOrientationRadians() const;

	// Rotations (return new Vec2)
	Vector2 GetRotatedBy90Degrees() const;      // Rotate (counter-clockwise)
	Vector2 GetRotatedByMinus90Degrees() const; // Rotate (clockwise)
	Vector2 GetRotatedByDegrees(float degrees) const;
	Vector2 GetRotatedByRadians(float radians) const;

	// Mutators (change this Vec2)
	void SetOrientationDegrees(float degrees);
	void SetOrientationRadians(float radians);
	void SetPolarDegrees(float degrees, float length);
	void SetPolarRadians(float radians, float length);
	void RotateDegrees(float degrees);
	void RotateRadians(float radians);
	void Rotate90Degrees();
	void RotateMinus90Degrees();

	// Length/Normalization
	Vector2 GetClamped(float maxLength) const;
	Vector2 GetNormalized() const;
	void    SetLength(float newLength);
	void    ClampLength(float maxLength);
	void    Normalize();
	float   NormalizeAndGetPreviousLength();

	// Reflection
	Vector2 const GetReflected(Vector2 const& normal) const;
	void          Reflect(Vector2 const& normal);

	void SetFromText(char const* text);

public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float x = 0.f;
	float y = 0.f;

	// Zero vector constant
	static const Vector2 Zero;
	static const Vector2 One;
};

#pragma once

struct Vec3;

//-----------------------------------------------------------------------------------------------
struct Vec2
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float x = 0.f;
	float y = 0.f;

	// Zero vector constant
	static const Vec2 ZERO;

public:
	// Construction/Destruction
	~Vec2();												// destructor (do nothing)
	Vec2();													// default constructor (do nothing)
	Vec2( Vec2 const& copyFrom );							// copy constructor (from another vec2)
	Vec2( Vec3 const& copyFrom );                           // copy constructor (from Vec3, dropping Z)
	explicit Vec2( float initialX, float initialY );		// explicit constructor (from x, y)

	// Operators (const)
	bool		operator==( Vec2 const& compare ) const;		// vec2 == vec2
	bool		operator!=( Vec2 const& compare ) const;		// vec2 != vec2
	Vec2 const	operator+( Vec2 const& vecToAdd ) const;		// vec2 + vec2
	Vec2 const	operator-( Vec2 const& vecToSubtract ) const;	// vec2 - vec2
	Vec2 const	operator-() const;								// -vec2, i.e. "unary negation"
	Vec2 const	operator*( float uniformScale ) const;			// vec2 * float
	Vec2 const	operator*( Vec2 const& vecToMultiply ) const;	// vec2 * vec2
	Vec2 const	operator/( float inverseScale ) const;			// vec2 / float

	// Operators (self-mutating / non-const)
	void		operator+=( Vec2 const& vecToAdd );				// vec2 += vec2
	void		operator-=( Vec2 const& vecToSubtract );		// vec2 -= vec2
	void		operator*=( const float uniformScale );			// vec2 *= float
	void		operator/=( const float uniformDivisor );		// vec2 /= float
	void		operator=( Vec2 const& copyFrom );				// vec2 = vec2

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec2::
	friend Vec2 const operator*( float uniformScale, Vec2 const& vecToScale );	// float * vec2

	// Static factory methods
	static Vec2 MakeFromPolarDegrees(float degrees, float length = 1.f);
	static Vec2 MakeFromPolarRadians(float radians, float length = 1.f);

	// Getters
	float GetLength() const;
	float GetLengthSquared() const;
	float GetOrientationDegrees() const;
	float GetOrientationRadians() const;

	// Rotations (return new Vec2)
	Vec2 GetRotatedBy90Degrees() const;
	Vec2 GetRotatedByMinus90Degrees() const;
	Vec2 GetRotatedByDegrees(float degrees) const;
	Vec2 GetRotatedByRadians(float radians) const;

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
	Vec2 GetClamped(float maxLength) const;
	Vec2 GetNormalized() const;
	void SetLength(float newLength);
	void ClampLength(float maxLength);
	void Normalize();
	float NormalizeAndGetPreviousLength();

	// Reflection
	Vec2 const	GetReflected(Vec2 const& normal) const;
	void		Reflect(Vec2 const& normal);
};
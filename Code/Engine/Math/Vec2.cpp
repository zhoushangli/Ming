#include "Engine/Math/Vec2.hpp"
//#include "Engine/Math/MathUtils.hpp"
//#include "Engine/Core/EngineCommon.hpp"


//-----------------------------------------------------------------------------------------------
Vec2::Vec2( Vec2 const& copy )
	: x( copy.x )
	, y( copy.y )
{
}


//-----------------------------------------------------------------------------------------------
Vec2::Vec2( float initialX, float initialY )
	: x( initialX )
	, y( initialY )
{
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator + ( Vec2 const& vecToAdd ) const
{
	return Vec2( this->x + vecToAdd.x, this->y + vecToAdd.y );
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-( Vec2 const& vecToSubtract ) const
{
	return Vec2(this->x - vecToSubtract.x, this->y - vecToSubtract.y);
}


//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-() const
{
	return Vec2( -this->x, -this->y );
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*( float uniformScale ) const
{
	return Vec2( this->x * uniformScale, this->y * uniformScale );
}


//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*( Vec2 const& vecToMultiply ) const
{
	return Vec2(this->x * vecToMultiply.x, this->y * vecToMultiply.y);
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator/( float inverseScale ) const
{
	return Vec2(this->x / inverseScale, this->y / inverseScale);
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator+=( Vec2 const& vecToAdd )
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator-=( Vec2 const& vecToSubtract )
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator*=( const float uniformScale )
{
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator/=( const float uniformDivisor )
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator=( Vec2 const& copyFrom )
{
	x = copyFrom.x;
	y = copyFrom.y;
}


//-----------------------------------------------------------------------------------------------
Vec2 const operator*( float uniformScale, Vec2 const& vecToScale )
{
	return Vec2(vecToScale.x * uniformScale, vecToScale.y * uniformScale);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator==( Vec2 const& compare ) const
{
	return x == compare.x && y == compare.y;
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator!=( Vec2 const& compare ) const
{
	return x != compare.x || y != compare.y;
}



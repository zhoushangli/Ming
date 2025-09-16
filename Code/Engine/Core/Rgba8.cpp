#include "Rgba8.hpp"

Rgba8 const Rgba8::WHITE = Rgba8(255, 255, 255, 255);
Rgba8 const Rgba8::BLACK = Rgba8(0, 0, 0, 255);

Rgba8::Rgba8()
	: r(255)
	, g(255)
	, b(255)
	, a(255)
{
}

Rgba8::Rgba8(unsigned char red, unsigned char green, unsigned char blue) 
	: r(red), g(green), b(blue), a(255)
{
}

Rgba8::Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) 
	: r(red), g(green), b(blue), a(alpha)
{
}

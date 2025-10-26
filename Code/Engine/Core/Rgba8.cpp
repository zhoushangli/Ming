#include "Rgba8.hpp"

Rgba8 const Rgba8::WHITE	= Rgba8(255, 255, 255);
Rgba8 const Rgba8::BLACK	= Rgba8(0, 0, 0);
Rgba8 const Rgba8::RED		= Rgba8(255, 0, 0);
Rgba8 const Rgba8::GREEN	= Rgba8(0, 255, 0);
Rgba8 const Rgba8::BLUE		= Rgba8(0, 0, 255);
Rgba8 const Rgba8::YELLOW	= Rgba8(255, 255, 0);
Rgba8 const Rgba8::CYAN		= Rgba8(0, 255, 255);
Rgba8 const Rgba8::MAGENTA	= Rgba8(255, 0, 255);
Rgba8 const Rgba8::ORANGE	= Rgba8(255, 165, 0);
Rgba8 const Rgba8::PURPLE	= Rgba8(128, 0, 128);
Rgba8 const Rgba8::PINK		= Rgba8(255, 182, 193);
Rgba8 const Rgba8::GRAY		= Rgba8(128, 128, 128);
Rgba8 const Rgba8::LIGHT_GRAY = Rgba8(192, 192, 192);
Rgba8 const Rgba8::DARK_GRAY = Rgba8(64, 64, 64);
Rgba8 const Rgba8::TRANSPARENT = Rgba8(0, 0, 0, 0);

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

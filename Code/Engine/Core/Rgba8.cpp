#include "Rgba8.hpp"

Rgba8::Rgba8()
	: r(255)
	, g(255)
	, b(255)
	, a(255)
{
}

Rgba8::Rgba8(char red, char green, char blue) : r(red), g(green), b(blue), a(255)
{
}

Rgba8::Rgba8(char red, char green, char blue, char alpha) : r(red), g(green), b(blue), a(alpha)
{
}

Rgba8::Rgba8(unsigned char initialR, unsigned char initialG, unsigned char initialB, unsigned char initialA)
{

}

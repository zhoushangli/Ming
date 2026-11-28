#include "MingEngine/Core/Render/Color.hpp"

#include "MingEngine/Core/StringUtils.hpp"

#include <algorithm>
#include <cmath>

Color const Color::White            = Color(1.f, 1.f, 1.f);
Color const Color::Black            = Color(0.f, 0.f, 0.f);
Color const Color::Red              = Color(1.f, 0.f, 0.f);
Color const Color::Green            = Color(0.f, 1.f, 0.f);
Color const Color::Blue             = Color(0.f, 0.f, 1.f);
Color const Color::Yellow           = Color(1.f, 1.f, 0.f);
Color const Color::Cyan             = Color(0.f, 1.f, 1.f);
Color const Color::Magenta          = Color(1.f, 0.f, 1.f);
Color const Color::Orange           = Color(1.f, 165.f / 255.f, 0.f);
Color const Color::Purple           = Color(128.f / 255.f, 0.f, 128.f / 255.f);
Color const Color::Pink             = Color(1.f, 182.f / 255.f, 193.f / 255.f);
Color const Color::Gray             = Color(128.f / 255.f, 128.f / 255.f, 128.f / 255.f);
Color const Color::LightGray        = Color(176.f / 255.f, 176.f / 255.f, 176.f / 255.f);
Color const Color::DarkGray         = Color(32.f / 255.f, 32.f / 255.f, 32.f / 255.f);
Color const Color::TranslucentBlack = Color(0.f, 0.f, 0.f, 128.f / 255.f);
Color const Color::Transparent      = Color(0.f, 0.f, 0.f, 0.f);

Color::Color() : r(1.f), g(1.f), b(1.f), a(1.f)
{
}

Color::Color(float red, float green, float blue) : r(red), g(green), b(blue), a(1.f)
{
}

Color::Color(float red, float green, float blue, float alpha)
	: r(red), g(green), b(blue), a(alpha)
{
}

bool Color::operator==(Color const& other) const
{
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator!=(Color const& other) const
{
	return !(*this == other);
}

Color const Color::operator+(float value) const
{
	return Color(
		r + value,
		g + value,
		b + value,
		a + value);
}

Color const Color::operator-(float value) const
{
	return Color(
		r - value,
		g - value,
		b - value,
		a - value);
}

Color const Color::operator*(float value) const
{
	return Color(
		r * value,
		g * value,
		b * value,
		a * value);
}

Color const Color::operator/(float value) const
{
	if (value == 0.f)
	{
		return *this;
	}

	return Color(
		r / value,
		g / value,
		b / value,
		a / value);
}

void Color::operator+=(float value)
{
	*this = *this + value;
}

void Color::operator-=(float value)
{
	*this = *this - value;
}

void Color::operator*=(float value)
{
	*this = *this * value;
}

void Color::operator/=(float value)
{
	*this = *this / value;
}

void Color::SetFromText(const char* text)
{
	Strings parts = SplitStringOnDelimiter(text, ',');

	if (parts.size() < 3)
	{
		r = 1.f;
		g = 1.f;
		b = 1.f;
		a = 1.f;
		return;
	}

	r = std::stof(parts[0]);
	g = std::stof(parts[1]);
	b = std::stof(parts[2]);
	if (parts.size() == 4)
	{
		a = std::stof(parts[3]);
	}
}

void Color::GetAsFloats(float* colorAsFloats) const
{
	colorAsFloats[0] = r;
	colorAsFloats[1] = g;
	colorAsFloats[2] = b;
	colorAsFloats[3] = a;
}

float NormalizeByte(unsigned char byteValue)
{
	return static_cast<float>(byteValue) / 255.f;
}

unsigned char DenormalizeByte(float normalizedValue)
{
	return static_cast<unsigned char>(std::lround(std::clamp(normalizedValue, 0.f, 1.f) * 255.f));
}

Color Color::Clamped() const
{
	return Color(std::clamp(r, 0.f, 1.f), std::clamp(g, 0.f, 1.f), std::clamp(b, 0.f, 1.f), std::clamp(a, 0.f, 1.f));
}

Color const operator+(float value, Color const& color)
{
	return color + value;
}

Color const operator-(float value, Color const& color)
{
	return Color(
		value - color.r,
		value - color.g,
		value - color.b,
		value - color.a);
}

Color const operator*(float value, Color const& color)
{
	return color * value;
}

Color const operator/(float value, Color const& color)
{
	return Color(
		color.r == 0.f ? 1.f : value / color.r,
		color.g == 0.f ? 1.f : value / color.g,
		color.b == 0.f ? 1.f : value / color.b,
		color.a == 0.f ? 1.f : value / color.a);
}

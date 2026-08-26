#include "MingEngine/Core/Render/Color.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/StringUtils.hpp"

using namespace Math;

Color const Color::White            = Color(255, 255, 255);
Color const Color::Black            = Color(0, 0, 0);
Color const Color::Red              = Color(255, 0, 0);
Color const Color::Green            = Color(0, 255, 0);
Color const Color::Blue             = Color(0, 0, 255);
Color const Color::Yellow           = Color(255, 255, 0);
Color const Color::Cyan             = Color(0, 255, 255);
Color const Color::Magenta          = Color(255, 0, 255);
Color const Color::Orange           = Color(255, 165, 0);
Color const Color::Purple           = Color(128, 0, 128);
Color const Color::Pink             = Color(255, 182, 193);
Color const Color::Gray             = Color(128, 128, 128);
Color const Color::LightGray        = Color(176, 176, 176);
Color const Color::DarkGray         = Color(32, 32, 32);
Color const Color::TranslucentBlack = Color(0, 0, 0, 128);
Color const Color::Transparent      = Color(0, 0, 0, 0);

Color::Color() : r(255), g(255), b(255), a(255) {}

Color::Color(unsigned char red, unsigned char green, unsigned char blue) : r(red), g(green), b(blue), a(255) {}

Color::Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
	: r(red), g(green), b(blue), a(alpha)
{
}

bool Color::operator==(Color const& other) const
{
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator!=(Color const& other) const { return !(*this == other); }

Color const Color::operator+(float value) const
{
	return Color(
		static_cast<unsigned char>(GetClamped(static_cast<float>(r) + value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) + value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) + value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) + value, 0.f, 255.f)));
}

Color const Color::operator-(float value) const
{
	return Color(
		static_cast<unsigned char>(GetClamped(static_cast<float>(r) - value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) - value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) - value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) - value, 0.f, 255.f)));
}

Color const Color::operator*(float value) const
{
	return Color(
		static_cast<unsigned char>(GetClamped(static_cast<float>(r) * value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) * value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) * value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) * value, 0.f, 255.f)));
}

Color const Color::operator/(float value) const
{
	if (value == 0.f)
	{
		return *this;
	}

	return Color(
		static_cast<unsigned char>(GetClamped(static_cast<float>(r) / value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) / value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) / value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) / value, 0.f, 255.f)));
}

void Color::operator+=(float value) { *this = *this + value; }

void Color::operator-=(float value) { *this = *this - value; }

void Color::operator*=(float value) { *this = *this * value; }

void Color::operator/=(float value) { *this = *this / value; }

void Color::SetFromText(const char* text)
{
	Strings parts = SplitStringOnDelimiter(text, ',');

	if (parts.size() < 3)
	{
		r = 255;
		g = 255;
		b = 255;
		a = 255;
		return;
	}

	r = static_cast<unsigned char>(atoi(parts[0].c_str()));
	g = static_cast<unsigned char>(atoi(parts[1].c_str()));
	b = static_cast<unsigned char>(atoi(parts[2].c_str()));
	if (parts.size() == 4)
	{
		a = static_cast<unsigned char>(std::stoi(parts[3]));
	}
}

void Color::GetAsFloats(float* colorAsFloats) const
{
	colorAsFloats[0] = NormalizeByte(r);
	colorAsFloats[1] = NormalizeByte(g);
	colorAsFloats[2] = NormalizeByte(b);
	colorAsFloats[3] = NormalizeByte(a);
}

float NormalizeByte(unsigned char byteValue) { return static_cast<float>(byteValue) / 255.f; }

unsigned char DenormalizeByte(float normalizedValue)
{
	float v = normalizedValue * 256.f;
	return static_cast<unsigned char>(GetClamped(v, 0.f, 255.f));
}

Color const operator+(float value, Color const& color) { return color + value; }

Color const operator-(float value, Color const& color)
{
	return Color(
		static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.r), 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.g), 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.b), 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.a), 0.f, 255.f)));
}

Color const operator*(float value, Color const& color) { return color * value; }

Color const operator/(float value, Color const& color)
{
	return Color(
		color.r == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.r), 0.f, 255.f)),
		color.g == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.g), 0.f, 255.f)),
		color.b == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.b), 0.f, 255.f)),
		color.a == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.a), 0.f, 255.f)));
}

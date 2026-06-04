#include "Engine/Core/Rgba8.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Rgba8.hpp"

Rgba8 const Rgba8::White            = Rgba8(255, 255, 255);
Rgba8 const Rgba8::Black            = Rgba8(0, 0, 0);
Rgba8 const Rgba8::Red              = Rgba8(255, 0, 0);
Rgba8 const Rgba8::Green            = Rgba8(0, 255, 0);
Rgba8 const Rgba8::Blue             = Rgba8(0, 0, 255);
Rgba8 const Rgba8::Yellow           = Rgba8(255, 255, 0);
Rgba8 const Rgba8::Cyan             = Rgba8(0, 255, 255);
Rgba8 const Rgba8::Magenta          = Rgba8(255, 0, 255);
Rgba8 const Rgba8::Orange           = Rgba8(255, 165, 0);
Rgba8 const Rgba8::Purple           = Rgba8(128, 0, 128);
Rgba8 const Rgba8::Pink             = Rgba8(255, 182, 193);
Rgba8 const Rgba8::Gray             = Rgba8(128, 128, 128);
Rgba8 const Rgba8::LightGray        = Rgba8(176, 176, 176);
Rgba8 const Rgba8::DarkGray         = Rgba8(32, 32, 32);
Rgba8 const Rgba8::TranslucentBlack = Rgba8(0, 0, 0, 128);
Rgba8 const Rgba8::Transparent      = Rgba8(0, 0, 0, 0);

Rgba8::Rgba8() : r(255), g(255), b(255), a(255) {}

Rgba8::Rgba8(unsigned char red, unsigned char green, unsigned char blue) : r(red), g(green), b(blue), a(255) {}

Rgba8::Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
	: r(red), g(green), b(blue), a(alpha)
{
}

bool Rgba8::operator==(Rgba8 const& other) const
{
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Rgba8::operator!=(Rgba8 const& other) const { return !(*this == other); }

Rgba8 const Rgba8::operator+(float value) const
{
	return Rgba8(static_cast<unsigned char>(GetClamped(static_cast<float>(r) + value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) + value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) + value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) + value, 0.f, 255.f)));
}

Rgba8 const Rgba8::operator-(float value) const
{
	return Rgba8(static_cast<unsigned char>(GetClamped(static_cast<float>(r) - value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) - value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) - value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) - value, 0.f, 255.f)));
}

Rgba8 const Rgba8::operator*(float value) const
{
	return Rgba8(static_cast<unsigned char>(GetClamped(static_cast<float>(r) * value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) * value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) * value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) * value, 0.f, 255.f)));
}

Rgba8 const Rgba8::operator/(float value) const
{
	if (value == 0.f)
	{
		return *this;
	}

	return Rgba8(static_cast<unsigned char>(GetClamped(static_cast<float>(r) / value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(g) / value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(b) / value, 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(static_cast<float>(a) / value, 0.f, 255.f)));
}

void Rgba8::operator+=(float value) { *this = *this + value; }

void Rgba8::operator-=(float value) { *this = *this - value; }

void Rgba8::operator*=(float value) { *this = *this * value; }

void Rgba8::operator/=(float value) { *this = *this / value; }

void Rgba8::SetFromText(const char* text)
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

void Rgba8::GetAsFloats(float* colorAsFloats) const
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

Rgba8 const operator+(float value, Rgba8 const& color) { return color + value; }

Rgba8 const operator-(float value, Rgba8 const& color)
{
	return Rgba8(static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.r), 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.g), 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.b), 0.f, 255.f)),
		static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.a), 0.f, 255.f)));
}

Rgba8 const operator*(float value, Rgba8 const& color) { return color * value; }

Rgba8 const operator/(float value, Rgba8 const& color)
{
	return Rgba8(color.r == 0 ? 255
							  : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.r), 0.f, 255.f)),
		color.g == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.g), 0.f, 255.f)),
		color.b == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.b), 0.f, 255.f)),
		color.a == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.a), 0.f, 255.f)));
}

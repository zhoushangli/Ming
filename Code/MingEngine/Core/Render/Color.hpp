#pragma once

#include <cstddef>

#if defined(TRANSPARENT)
#undef TRANSPARENT
#endif

struct Color
{
public:
	Color();
	Color(float red, float green, float blue);
	Color(float red, float green, float blue, float alpha);
	~Color() = default;

	bool operator==(Color const& other) const;
	bool operator!=(Color const& other) const;

	Color const operator+(float value) const;
	Color const operator-(float value) const;
	Color const operator*(float value) const;
	Color const operator/(float value) const;

	void operator+=(float value);
	void operator-=(float value);
	void operator*=(float value);
	void operator/=(float value);

	void SetFromText(const char* text);
	void GetAsFloats(float* colorAsFloats) const;
	Color Clamped() const;

public:
	static const Color White;
	static const Color Black;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Magenta;
	static const Color Orange;
	static const Color Purple;
	static const Color Pink;
	static const Color Gray;
	static const Color LightGray;
	static const Color DarkGray;
	static const Color TranslucentBlack;
	static const Color Transparent;
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 0.f;
};

static_assert(sizeof(Color) == 16);
static_assert(offsetof(Color, r) == 0 && offsetof(Color, g) == 4 && offsetof(Color, b) == 8 && offsetof(Color, a) == 12);

float         NormalizeByte(unsigned char byteValue);
unsigned char DenormalizeByte(float normalizedValue);

Color const operator+(float value, Color const& color);
Color const operator-(float value, Color const& color);
Color const operator*(float value, Color const& color);
Color const operator/(float value, Color const& color);

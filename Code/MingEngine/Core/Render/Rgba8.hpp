#pragma once

#if defined(TRANSPARENT)
#undef TRANSPARENT
#endif

struct Color
{
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

	Color();
	Color(unsigned char red, unsigned char green, unsigned char blue);
	Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
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

public:
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
	unsigned char a = 0;
};

float         NormalizeByte(unsigned char byteValue);
unsigned char DenormalizeByte(float normalizedValue);

Color const operator+(float value, Color const& color);
Color const operator-(float value, Color const& color);
Color const operator*(float value, Color const& color);
Color const operator/(float value, Color const& color);

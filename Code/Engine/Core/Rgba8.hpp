#pragma once

#if defined(TRANSPARENT)
#undef TRANSPARENT
#endif

struct Rgba8
{
public:
	static const Rgba8 White;
	static const Rgba8 Black;
	static const Rgba8 Red;
	static const Rgba8 Green;
	static const Rgba8 Blue;
	static const Rgba8 Yellow;
	static const Rgba8 Cyan;
	static const Rgba8 Magenta;
	static const Rgba8 Orange;
	static const Rgba8 Purple;
	static const Rgba8 Pink;
	static const Rgba8 Gray;
	static const Rgba8 LightGray;
	static const Rgba8 DarkGray;
	static const Rgba8 TranslucentBlack;
	static const Rgba8 Transparent;

	Rgba8();
	Rgba8(unsigned char red, unsigned char green, unsigned char blue);
	Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
	~Rgba8() = default;

	bool operator==(Rgba8 const& other) const;
	bool operator!=(Rgba8 const& other) const;

	Rgba8 const operator+(float value) const;
	Rgba8 const operator-(float value) const;
	Rgba8 const operator*(float value) const;
	Rgba8 const operator/(float value) const;

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

Rgba8 const operator+(float value, Rgba8 const& color);
Rgba8 const operator-(float value, Rgba8 const& color);
Rgba8 const operator*(float value, Rgba8 const& color);
Rgba8 const operator/(float value, Rgba8 const& color);

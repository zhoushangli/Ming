#pragma once

#if defined(TRANSPARENT)
    #undef TRANSPARENT
#endif

struct Rgba8
{
public:
    static const Rgba8 kWhite;
    static const Rgba8 kBlack;
    static const Rgba8 kRed;
    static const Rgba8 kGreen;
    static const Rgba8 kBlue;
    static const Rgba8 kYellow;
    static const Rgba8 kCyan;
    static const Rgba8 kMagenta;
    static const Rgba8 kOrange;
    static const Rgba8 kPurple;
    static const Rgba8 kPink;
    static const Rgba8 kGray;
    static const Rgba8 kLightGray;
    static const Rgba8 kDarkGray;
    static const Rgba8 kTranslucentBlack;
    static const Rgba8 kTransparent;

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

float NormalizeByte(unsigned char byteValue);
unsigned char DenormalizeByte(float normalizedValue);

Rgba8 const operator+(float value, Rgba8 const& color);
Rgba8 const operator-(float value, Rgba8 const& color);
Rgba8 const operator*(float value, Rgba8 const& color);
Rgba8 const operator/(float value, Rgba8 const& color);

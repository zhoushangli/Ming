#pragma once

struct Rgba8
{
public:
    static const Rgba8 WHITE;
    static const Rgba8 BLACK;
    static const Rgba8 RED;
    static const Rgba8 GREEN;
    static const Rgba8 BLUE;
    static const Rgba8 YELLOW;
    static const Rgba8 CYAN;
    static const Rgba8 MAGENTA;
    static const Rgba8 ORANGE;
    static const Rgba8 PURPLE;
    static const Rgba8 PINK;
    static const Rgba8 GRAY;
    static const Rgba8 LIGHT_GRAY;
    static const Rgba8 DARK_GRAY;
    static const Rgba8 TRANSPARENT;

	Rgba8();
	Rgba8(unsigned char red, unsigned char green, unsigned char blue);
	Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
	~Rgba8() = default;

    void SetFromText(const char* text);

public:
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;
};

float NormalizeByte(unsigned char byteValue);
unsigned char DenormalizeByte(float normalizedValue);
Rgba8 Interpolate(Rgba8 const& start, Rgba8 const& end, float fraction);
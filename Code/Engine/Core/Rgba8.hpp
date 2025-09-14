#pragma once
struct Rgba8
{
public:
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
	unsigned char a = 0;
public:
	Rgba8();
	Rgba8(char red, char green, char blue);
	Rgba8(char red, char green, char blue, char alpha);
	~Rgba8() = default;
	explicit Rgba8(unsigned char initialR, unsigned char initialG, unsigned char initialB, unsigned char initialA);
};


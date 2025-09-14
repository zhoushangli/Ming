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
	Rgba8(unsigned char red, unsigned char green, unsigned char blue);
	Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
	~Rgba8() = default;
};


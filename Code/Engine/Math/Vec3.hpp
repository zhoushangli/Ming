#pragma once
struct Vec3
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

public:
	~Vec3() {}
	Vec3() {}
	Vec3(Vec3 const& copyFrom);
	explicit Vec3(float initialX, float initialY, float initialZ);
};


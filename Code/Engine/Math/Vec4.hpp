#pragma once

struct Vec4
{
public:
    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float xVal, float yVal, float zVal, float wVal) : x(xVal), y(yVal), z(zVal), w(wVal) {}

public:
    float x;
    float y;
    float z;
    float w;
};
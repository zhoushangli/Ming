#include "Engine/Core/Rgba8.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

Rgba8 const Rgba8::WHITE             = Rgba8(255, 255, 255);
Rgba8 const Rgba8::BLACK             = Rgba8(0, 0, 0);
Rgba8 const Rgba8::RED               = Rgba8(255, 0, 0);
Rgba8 const Rgba8::GREEN             = Rgba8(0, 255, 0);
Rgba8 const Rgba8::BLUE              = Rgba8(0, 0, 255);
Rgba8 const Rgba8::YELLOW            = Rgba8(255, 255, 0);
Rgba8 const Rgba8::CYAN              = Rgba8(0, 255, 255);
Rgba8 const Rgba8::MAGENTA           = Rgba8(255, 0, 255);
Rgba8 const Rgba8::ORANGE            = Rgba8(255, 165, 0);
Rgba8 const Rgba8::PURPLE            = Rgba8(128, 0, 128);
Rgba8 const Rgba8::PINK              = Rgba8(255, 182, 193);
Rgba8 const Rgba8::GRAY              = Rgba8(128, 128, 128);
Rgba8 const Rgba8::LIGHT_GRAY        = Rgba8(192, 192, 192);
Rgba8 const Rgba8::DARK_GRAY         = Rgba8(64, 64, 64);
Rgba8 const Rgba8::TRANSLUCENT_BLACK = Rgba8(0, 0, 0, 128);
Rgba8 const Rgba8::TRANSPARENT       = Rgba8(0, 0, 0, 0);

Rgba8::Rgba8()
    : r(255)
    , g(255)
    , b(255)
    , a(255)
{}

Rgba8::Rgba8(unsigned char red, unsigned char green, unsigned char blue)
    : r(red), g(green), b(blue), a(255)
{}

Rgba8::Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
    : r(red), g(green), b(blue), a(alpha)
{}

Rgba8 const Rgba8::operator+(float value) const
{
    return Rgba8(
        static_cast<unsigned char>(GetClamped(static_cast<float>(r) + value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(g) + value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(b) + value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(a) + value, 0.f, 255.f)));
}

Rgba8 const Rgba8::operator-(float value) const
{
    return Rgba8(
        static_cast<unsigned char>(GetClamped(static_cast<float>(r) - value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(g) - value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(b) - value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(a) - value, 0.f, 255.f)));
}

Rgba8 const Rgba8::operator*(float value) const
{
    return Rgba8(
        static_cast<unsigned char>(GetClamped(static_cast<float>(r) * value, 0.f, 255.f)),
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

    return Rgba8(
        static_cast<unsigned char>(GetClamped(static_cast<float>(r) / value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(g) / value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(b) / value, 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(static_cast<float>(a) / value, 0.f, 255.f)));
}

void Rgba8::operator+=(float value)
{
    *this = *this + value;
}

void Rgba8::operator-=(float value)
{
    *this = *this - value;
}

void Rgba8::operator*=(float value)
{
    *this = *this * value;
}

void Rgba8::operator/=(float value)
{
    *this = *this / value;
}

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

float NormalizeByte(unsigned char byteValue)
{
    return static_cast<float>(byteValue) / 255.f;
}

unsigned char DenormalizeByte(float normalizedValue)
{
    float v = normalizedValue * 256.f;
    return static_cast<unsigned char>(GetClamped(v, 0.f, 255.f));
}

Rgba8 Interpolate(Rgba8 const& start, Rgba8 const& end, float fraction)
{
    float r = Interpolate(NormalizeByte(start.r), NormalizeByte(end.r), fraction);
    float g = Interpolate(NormalizeByte(start.g), NormalizeByte(end.g), fraction);
    float b = Interpolate(NormalizeByte(start.b), NormalizeByte(end.b), fraction);
    float a = Interpolate(NormalizeByte(start.a), NormalizeByte(end.a), fraction);

    return Rgba8(DenormalizeByte(r), DenormalizeByte(g), DenormalizeByte(b), DenormalizeByte(a));
}

Rgba8 const operator+(float value, Rgba8 const& color)
{
    return color + value;
}

Rgba8 const operator-(float value, Rgba8 const& color)
{
    return Rgba8(
        static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.r), 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.g), 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.b), 0.f, 255.f)),
        static_cast<unsigned char>(GetClamped(value - static_cast<float>(color.a), 0.f, 255.f)));
}

Rgba8 const operator*(float value, Rgba8 const& color)
{
    return color * value;
}

Rgba8 const operator/(float value, Rgba8 const& color)
{
    return Rgba8(
        color.r == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.r), 0.f, 255.f)),
        color.g == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.g), 0.f, 255.f)),
        color.b == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.b), 0.f, 255.f)),
        color.a == 0 ? 255 : static_cast<unsigned char>(GetClamped(value / static_cast<float>(color.a), 0.f, 255.f)));
}

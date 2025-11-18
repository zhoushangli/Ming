#include "Engine/Core/Rgba8.hpp"

#include "Engine/Core/StringUtils.hpp"

Rgba8 const Rgba8::WHITE = Rgba8(255, 255, 255);
Rgba8 const Rgba8::BLACK = Rgba8(0, 0, 0);
Rgba8 const Rgba8::RED = Rgba8(255, 0, 0);
Rgba8 const Rgba8::GREEN = Rgba8(0, 255, 0);
Rgba8 const Rgba8::BLUE = Rgba8(0, 0, 255);
Rgba8 const Rgba8::YELLOW = Rgba8(255, 255, 0);
Rgba8 const Rgba8::CYAN = Rgba8(0, 255, 255);
Rgba8 const Rgba8::MAGENTA = Rgba8(255, 0, 255);
Rgba8 const Rgba8::ORANGE = Rgba8(255, 165, 0);
Rgba8 const Rgba8::PURPLE = Rgba8(128, 0, 128);
Rgba8 const Rgba8::PINK = Rgba8(255, 182, 193);
Rgba8 const Rgba8::GRAY = Rgba8(128, 128, 128);
Rgba8 const Rgba8::LIGHT_GRAY = Rgba8(192, 192, 192);
Rgba8 const Rgba8::DARK_GRAY = Rgba8(64, 64, 64);
Rgba8 const Rgba8::TRANSPARENT = Rgba8(0, 0, 0, 0);

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
